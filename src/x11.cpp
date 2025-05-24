#include <iostream>
#include <fstream>

#include <sys/time.h>
#include <fontconfig/fontconfig.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/xpm.h>

#include "main.h"
#include "x11.h"

#include "fonts/Ghoulish.h"
#include "fonts/Roboto.h"

#define X11_WIN_WIDTH 640
#define X11_WIN_HEIGHT 480

using namespace std;

const char* ttf_ghoulish = "/tmp/ghoulish.ttf";
const char* ttf_roboto = "/tmp/roboto.ttf";

Window window = None;
Pixmap pixmap_qr = None;
Colormap colormap = None;
Display* display = nullptr;
XftFont* xft_hdr_font = nullptr;
XftFont* xft_std_font = nullptr;
XftFont* xft_sub_font = nullptr;
XftDraw* xft_draw = nullptr;
Visual* visual = nullptr;
GC gc = nullptr;
XftColor xft_color = {0};
XSizeHints hints = {0};

static void x11Init()
{
  XInitThreads();
  setenv("DISPLAY", ":0", 0);
  display = XOpenDisplay(nullptr);
  if (!display) {
    cerr << "Failed to open X11 display." << endl;
  }
}

static void loadFont(const char* ttfpath, const unsigned char* ttfdata, const unsigned int ttfsize, FcConfig* config)
{
  ofstream font_file(ttfpath, ios::binary);
  if (!font_file) {
    cerr << "Failed to open font file for writing." << endl;
    exit(EXIT_FAILURE);
  }

  font_file.write(reinterpret_cast<const char*>(ttfdata), ttfsize);
  font_file.close();

  if (!FcConfigAppFontAddFile(config, (const FcChar8*)ttfpath)) {
    cerr << "Failed to add font file to Fontconfig." << endl;
    exit(EXIT_FAILURE);
  }
}

static void drawPlayerList()
{
  XCopyArea(display, pixmap_qr, window, gc, 0, 0, 145, 145, X11_WIN_WIDTH - 145, 0);
  XClearArea(display, window, 0, 175, X11_WIN_WIDTH, 50 * 4, 0);

  XftDrawString8(xft_draw, &xft_color, xft_hdr_font,
    10, 50, (const FcChar8*)"Spooky Scoreboard", 17);

  XftDrawString8(xft_draw, &xft_color, xft_sub_font,
    15, 80, (const FcChar8*)machineUrl.c_str(), machineUrl.size());

  XftDrawString8(xft_draw, &xft_color, xft_sub_font,
    X11_WIN_WIDTH - 70, X11_WIN_HEIGHT - 10, (const FcChar8*)VERSION, strlen(VERSION));

  for (int i = 0, ty = 175; i < playerList.player.size(); i++, ty += 75) {
    char name[60];
    snprintf(name, sizeof(name), "%d. %s", i + 1, playerList.player[i].c_str());
    XftDrawString8(xft_draw, &xft_color, xft_std_font, 10, ty, (const FcChar8*)name, strlen(name));
  }
}

void runTimer(int secs)
{
  struct timeval start_time, current_time, update_time;
  gc = XCreateGC(display, window, 0, NULL);

  gettimeofday(&start_time, NULL);
  update_time = start_time;

  while (1) {
    XEvent event;

    while (XPending(display) > 0) {
      XNextEvent(display, &event);
      if (event.type == Expose && event.xexpose.count == 0) drawPlayerList();
    }

    gettimeofday(&current_time, NULL);
    if (current_time.tv_sec - start_time.tv_sec >= secs) break;

    if (current_time.tv_sec - update_time.tv_sec >= 1) {
      char ct[3];

      snprintf(
        ct,
        sizeof(ct),
        "%d",
        (int)(secs - (current_time.tv_sec - start_time.tv_sec)));

      XClearArea(display, window, 0, X11_WIN_HEIGHT - 60, 100, 100, 1);

      XftDrawString8(
        xft_draw,
        &xft_color,
        xft_sub_font,
        10,
        X11_WIN_HEIGHT - 10,
        (FcChar8*)ct,
        strlen(ct));

      update_time = current_time;
    }
  }

  XFreeGC(display, gc);
}

void showPlayerListWindow()
{
  if (window != None) {
    cout << "Showing player list window." << endl;
    XMapWindow(display, window);
    XRaiseWindow(display, window);
    game->sendi3cmd();
    XSync(display, False);
  }
}

void hidePlayerListWindow()
{
  if (window != None) {
    cout << "Hiding player list window." << endl;
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

    remove(ttf_ghoulish);
    remove(ttf_roboto);

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
    WhitePixel(display, screen));

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

  int rc = XpmReadFileToPixmap(display, window, "/game/tmp/qrcode.xpm", &pixmap_qr, NULL, NULL);
  if (rc != XpmSuccess) {
    cerr << "Failed to create pixmap." << XpmGetErrorString(rc) << endl;
  }

  FcConfig* fc_config = FcInitLoadConfigAndFonts();
  loadFont(ttf_ghoulish, Ghoulish_ttf, Ghoulish_ttf_len, fc_config);
  loadFont(ttf_roboto, Roboto_ttf, Roboto_ttf_len, fc_config);
  FcConfigSetCurrent(fc_config);

  xft_std_font = XftFontOpenName(display, screen, "Ghoulish:size=26");
  if (!xft_std_font) {
    cerr << "Failed to open standard TTF font." << endl;
    exit(EXIT_FAILURE);
  }

  xft_hdr_font = XftFontOpenName(display, screen, "Ghoulish:size=38");
  if (!xft_hdr_font) {
    cerr << "Failed to open header TTF font." << endl;
    exit(EXIT_FAILURE);
  }

  xft_sub_font = XftFontOpenName(display, screen, "Roboto:size=16");
  if (!xft_sub_font) {
    cerr << "Failed to open sub TTF font." << endl;
    exit(EXIT_FAILURE);
  }

  colormap = DefaultColormap(display, screen);
  visual = DefaultVisual(display, screen);

  XftColorAllocName(
    display,
    visual,
    colormap,
    "black",
    &xft_color);

  xft_draw = XftDrawCreate(
    display,
    window,
    visual,
    colormap);

  XSelectInput(display, window, ExposureMask);
}

// vim: set ts=2 sw=2 expandtab:

