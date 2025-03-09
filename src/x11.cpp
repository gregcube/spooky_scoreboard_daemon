#include <iostream>

#include <sys/time.h>
#include <fontconfig/fontconfig.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/xpm.h>

#include "main.h"
#include "x11.h"

#define X11_WIN_WIDTH 640
#define X11_WIN_HEIGHT 480

Display* display = nullptr;
Window window = None;
XftFont* xft_hdr_font = nullptr;
XftFont* xft_std_font = nullptr;
XftFont* xft_sub_font = nullptr;
XftDraw* xft_draw = nullptr;
XftColor xft_color = {0};
XSizeHints hints = {0};
Colormap colormap;
Visual* visual = nullptr;
Pixmap pixmap_qr = None;

void x11Init()
{
  XInitThreads();
  setenv("DISPLAY", ":0", 0);
  display = XOpenDisplay(nullptr);
  if (!display) {
    std::cerr << "Failed to open X11 display." << std::endl;
  }
}

void runTimer(int secs)
{
  struct timeval start_time, current_time, update_time;
  GC gc = XCreateGC(display, window, 0, NULL);

  gettimeofday(&start_time, NULL);
  update_time = start_time;

  while (1) {
    XEvent event;

    while (XPending(display) > 0) {
      XNextEvent(display, &event);

      if (event.type == Expose && event.xexpose.count == 0) {
        XCopyArea(display, pixmap_qr, window, gc, 0, 0, 145, 145, X11_WIN_WIDTH - 145, 0);
        XClearArea(display, window, 0, 175, X11_WIN_WIDTH, 50 * 4, 0);

        XftDrawString8(xft_draw, &xft_color, xft_hdr_font, 10, 40, (const FcChar8*)"Spooky Scoreboard", 17);
        XftDrawString8(xft_draw, &xft_color, xft_sub_font, 10, 60, (const FcChar8*)"spookyscoreboard.com", 20);

        for (int i = 0, ty = 175; i < playerList.player.size(); i++, ty += 75) {
          char username[60];

          snprintf(
            username,
            sizeof(username),
            "%d. %s",
            i + 1,
            playerList.player[i].c_str());

          XftDrawString8(
            xft_draw,
            &xft_color,
            xft_std_font,
            10,
            ty,
            (XftChar8*)username,
            strlen(username));
        }
      }
    }

    gettimeofday(&current_time, NULL);
    if (current_time.tv_sec - start_time.tv_sec >= secs) {
      break;
    }

    if (current_time.tv_sec - update_time.tv_sec >= 1) {
      char ct[3];

      snprintf(
        ct,
        sizeof(ct),
        "%d",
        (int)(secs - (current_time.tv_sec - start_time.tv_sec)));

      XClearArea(display, window, 0, X11_WIN_HEIGHT - 50, 100, 100, 1);

      XftDrawString8(
        xft_draw,
        &xft_color,
        xft_std_font,
        10,
        X11_WIN_HEIGHT - 10,
        (XftChar8 *)ct,
        strlen(ct));

      update_time = current_time;
    }
  }

  XFreeGC(display, gc);
}

void showPlayerListWindow()
{
  if (window != None) {
    std::cout << "Showing player list window." << std::endl;
    XMapWindow(display, window);
    XRaiseWindow(display, window);
    game->sendi3cmd();
    XSync(display, False);
  }
}

void hidePlayerListWindow()
{
  if (window != None) {
    std::cout << "Hiding player list window." << std::endl;
    XUnmapWindow(display, window);
    XSync(display, False);
  }
}

void closePlayerListWindow()
{
  if (display != nullptr) {
    if (pixmap_qr != None)
      XFreePixmap(display, pixmap_qr);

    if (xft_draw != nullptr)
      XftDrawDestroy(xft_draw);

    if (visual != nullptr)
      XftColorFree(display, visual, colormap, &xft_color);

    if (xft_std_font != nullptr)
      XftFontClose(display, xft_std_font);

    if (xft_hdr_font != nullptr)
      XftFontClose(display, xft_hdr_font);

    if (xft_sub_font != nullptr)
      XftFontClose(display, xft_sub_font);

    if (window != None)
      XDestroyWindow(display, window);

    XFlush(display);
    XCloseDisplay(display);
    display = nullptr;
  }
}

void openPlayerListWindow()
{
  if (display == nullptr) x11Init();

  int screen = DefaultScreen(display);

  int w = DisplayWidth(display, screen);
  int h = DisplayHeight(display, screen);

  int ww = X11_WIN_WIDTH;
  int wh = X11_WIN_HEIGHT;

  window = XCreateSimpleWindow(
    display,
    RootWindow(display, screen),
    (w - ww) / 2,
    (h - wh) / 2,
    ww,
    wh,
    0,
    BlackPixel(display, screen),
    WhitePixel(display, screen)
  );

  hints.flags = PSize | PMinSize | PMaxSize | PPosition;
  hints.width = hints.base_width = hints.min_width = hints.max_width = ww;
  hints.height = hints.base_height = hints.min_height = hints.max_height = wh;
  hints.x = (w - ww) / 2;
  hints.y = (h - wh) / 2;

  XSetWMNormalHints(display, window, &hints);
  XStoreName(display, window, "SSBd");

  Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
  Atom wm_state_above = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
  XChangeProperty(
    display,
    window,
    wm_state,
    XA_ATOM,
    32,
    PropModeReplace,
    (unsigned char*)&wm_state_above,
    1);

  Atom opacity_atom = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", False);
  unsigned long opacity = (unsigned long)(0xffffffff * 0.75);
  XChangeProperty(
    display,
    window,
    opacity_atom,
    XA_CARDINAL,
    32,
    PropModeReplace,
    (unsigned char *)&opacity,
    1);

  int rc = XpmReadFileToPixmap(display, window, "/game/tmp/qrcode.xpm", &pixmap_qr, NULL, NULL);
  if (rc != XpmSuccess) {
    std::cerr << "Failed to create pixmap." << XpmGetErrorString(rc) << std::endl;
  }

  FcBool result = FcConfigAppFontAddFile(
    FcConfigGetCurrent(),
    (const FcChar8*)"/game/code/assets/fonts/SNNeoNoire-Regular.ttf"
  );

  if (!result) {
    std::cerr << "Failed to load font." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  xft_std_font = XftFontOpenName(display, screen, "SN NeoNoire:size=18");
  if (!xft_std_font) {
    std::cerr << "Failed to open standard TTF font." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  xft_hdr_font = XftFontOpenName(display, screen, "SN NeoNoire:size=24");
  if (!xft_hdr_font) {
    std::cerr << "Failed to open header TTF font." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  xft_sub_font = XftFontOpenName(display, screen, "SN NeoNoire:size=10");
  if (!xft_sub_font) {
    std::cerr << "Failed to open sub TTF font." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  colormap = DefaultColormap(display, screen);
  visual = DefaultVisual(display, screen);

  XftColorAllocName(
    display,
    visual,
    colormap,
    "black",
    &xft_color
  );

  xft_draw = XftDrawCreate(
    display,
    window,
    visual,
    colormap
  );

  XSelectInput(display, window, ExposureMask);
}

// vim: set ts=2 sw=2 expandtab:

