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
#define X11_WIN_HEIGHT 330

Display* display = nullptr;
Window window = None;
XftFont* xft_font = nullptr;
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
        XCopyArea(display, pixmap_qr, window, gc, 0, 0, 130, 130, 500, 195);
        XClearArea(display, window, 0, 0, 640, 50 * 4, 0);

        for (int i = 0, ty = 30; i < playerList.player.size(); i++, ty += 50) {
          char username[60];

          snprintf(
            username,
            sizeof(username),
            "Player %d: %s",
            i + 1,
            playerList.player[i].c_str());

          XftDrawString8(
            xft_draw,
            &xft_color,
            xft_font,
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

      XClearArea(display, window, 0, 280, 50, 50, 1);

      XftDrawString8(
        xft_draw,
        &xft_color,
        xft_font,
        10,
        320,
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

    if (xft_font != nullptr)
      XftFontClose(display, xft_font);

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
    1
  );

  int rc = XpmReadFileToPixmap(display, window, "/game/tmp/qrcode.xpm", &pixmap_qr, NULL, NULL);
  if (rc != XpmSuccess) {
    std::cerr << "Failed to create pixmap." << XpmGetErrorString(rc) << std::endl;
  }

  FcBool result = FcConfigAppFontAddFile(
    FcConfigGetCurrent(),
    (const FcChar8*)"/game/code/assets/fonts/Atari_Hanzel.ttf"
  );

  if (!result) {
    std::cerr << "Failed to load font." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  xft_font = XftFontOpenName(display, screen, "Hanzel:size=21");
  if (!xft_font) {
    std::cerr << "Failed to open TTF font." << std::endl;
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

