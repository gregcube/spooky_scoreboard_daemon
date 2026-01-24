// Spooky Scoreboard Daemon
// Copyright (C) 2025 Greg MacKenzie
// https://spookyscoreboard.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstring>

#include <sys/time.h>
#include <fontconfig/fontconfig.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/xpm.h>

#include "main.h"
#include "x11.h"
#include "version.h"

#include "font/Ghoulish.h"
#include "font/Roboto.h"

#define X11_WIN_WIDTH 320
#define X11_WIN_HEIGHT 480
#define X11_WIN_GAP 10

using namespace std;

Window window[5] = {None, None, None, None, None};
GC gc[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
XftDraw* xft_draw[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
Pixmap pixmap_buf[5] = {None, None, None, None, None};
Pixmap pixmap_qr = None;
Colormap colormap = None;
Display* display = nullptr;
XftFont* xft_hdr_font = nullptr;
XftFont* xft_std_font = nullptr;
XftFont* xft_sub_font = nullptr;
Visual* visual = nullptr;
XftColor xft_color = {0, 0, 0, 0, 0};

mutex timer_mtx, thread_mtx;
vector<bool> windowThread = vector<bool>(5, false);

/**
 * Initializes the X11 display connection and set up the display environment.
 * This function must be called before any other X11 operations.
 */
void x11Init()
{
  XInitThreads();
  setenv("DISPLAY", ":0", 0);
  display = XOpenDisplay(nullptr);
  if (!display) {
    cerr << "Failed to open X11 display." << endl;
    exit(EXIT_FAILURE);
  }
}

/**
 * Load a TrueType font into the Fontconfig system.
 *
 * @param path Path where the font file will be temporarily stored.
 * @param data Raw font data to be written.
 * @param size Size of the font data in bytes.
 * @param config Fontconfig configuration object.
 */
static void loadFont(const char* path, const unsigned char* data, size_t size, FcConfig* config)
{
  ofstream font_file(path, ios::binary);
  if (!font_file) {
    cerr << "Failed to open font file for writing." << endl;
    exit(EXIT_FAILURE);
  }

  font_file.write(reinterpret_cast<const char*>(data), size);
  font_file.close();

  if (!FcConfigAppFontAddFile(config, (const FcChar8*)path)) {
    cerr << "Failed to add font file to Fontconfig." << endl;
    exit(EXIT_FAILURE);
  }
}

/**
 * Create an X11 window.
 */
static Window createWindow(int x, int y, int w, int h, const string& title, int scr)
{
  Window win = XCreateSimpleWindow(
    display,
    RootWindow(display, scr),
    x, y, w, h, 0, BlackPixel(display, scr), WhitePixel(display, scr));

  XSizeHints hints;
  memset(&hints, 0, sizeof(XSizeHints));
  hints.flags = PSize | PMinSize | PMaxSize | PPosition;
  hints.width = hints.base_width = hints.min_width = hints.max_width = w;
  hints.height = hints.base_height = hints.min_height = hints.max_height = h;
  hints.x = x;
  hints.y = y;
  XSetWMNormalHints(display, win, &hints);

  XStoreName(display, win, title.c_str());

  Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
  Atom wm_state_above = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
  XChangeProperty(
    display,
    win,
    wm_state,
    XA_ATOM,
    32,
    PropModeReplace,
    (unsigned char*)&wm_state_above,
    1);

  return win;
}

static vector<string> wrapText(const string& text, XftFont* font, int max_pixel_width)
{
  if (text.empty() || max_pixel_width <= 0) return {};

  vector<string> lines;
  istringstream stream(text);
  string word, current;

  auto width_of = [&](const string& s) -> int {
    if (s.empty()) return 0;

    XGlyphInfo gi;
    XftTextExtents8(display,
                    font,
                    reinterpret_cast<const FcChar8*>(s.c_str()),
                    static_cast<int>(s.length()),
                    &gi);

    return gi.xOff;
  };

  while (stream >> word) {
    // Single long word wrap.
    if (width_of(word) > max_pixel_width) {
      if (!current.empty()) {
        lines.emplace_back(move(current));
        current.clear();
      }

      string frag;
      for (char c : word) {
        string test = frag + c;
        if (width_of(test) > max_pixel_width && !frag.empty()) {
          lines.emplace_back(move(frag));
          frag = c;
        }
        else {
          frag = move(test);
        }
      }

      current = move(frag);
      continue;
    }

    // Normal word-based wrap.
    string test = current.empty() ? word : current + " " + word;
    if (width_of(test) > max_pixel_width) {
      if (!current.empty()) {
        lines.emplace_back(move(current));
        current.clear();
      }
      current = move(word);
    }
    else {
      current = move(test);
    }
  }

  if (!current.empty()) {
    lines.emplace_back(move(current));
  }

  return lines;
}

/**
 * Draws the content for a specific window.
 *
 * @param index The index of the window to draw (0-4).
 *              0-3: player windows
 *                4: message window
 */
void drawWindow(int index)
{
  if (index < 0 || index > 4 || window[index] == None) {
    cerr << "Invalid window index: " << index << endl;
    return;
  }

  if (index < 4 && playerList.player[index].empty()) return;

  int screen = DefaultScreen(display);

  XGlyphInfo ext;
  XWindowAttributes attr;
  XGetWindowAttributes(display, window[index], &attr);

  int w = attr.width;
  int h = attr.height;
  int center_x = w / 2;
  int header_y;

  // Create a white rectangle for centered content.
  XSetForeground(display, gc[index], WhitePixel(display, screen));
  XFillRectangle(display, pixmap_buf[index], gc[index], 0, 0, w, h - xft_sub_font->height);

  // Set foreground for black text.
  XSetForeground(display, gc[index], BlackPixel(display, screen));

  // Draw "Spooky" text.
  const char* spooky = "Spooky";
  header_y = xft_hdr_font->ascent;
  XftTextExtents8(display, xft_hdr_font, (FcChar8*)spooky, 6, &ext);
  XftDrawString8(xft_draw[index], &xft_color, xft_hdr_font,
                 center_x - ext.width / 2, header_y,
                 (const FcChar8*)spooky, 6);

  // Draw "Scoreboard" text.
  const char* scoreboard = "Scoreboard";
  header_y += xft_hdr_font->height;
  XftTextExtents8(display, xft_hdr_font, (FcChar8*)scoreboard, 10, &ext);
  XftDrawString8(xft_draw[index], &xft_color, xft_hdr_font,
                 center_x - ext.width / 2, header_y,
                 (const FcChar8*)scoreboard, 10);

  // Draw QR code.
  int qr_x = center_x - 72; // 145/2 = 72
  int qr_y = header_y + 10;
  XCopyArea(display, pixmap_qr, pixmap_buf[index], gc[index], 0, 0, 145, 145, qr_x, qr_y);

  // Main text area.
  int text_margin_x = 40;
  int max_text_w = w - 2 * text_margin_x;
  int text_area_top = qr_y + 145;
  int text_area_h = h - text_area_top - 60;

  string text = (index < 4) ? playerList.player[index] : serverMessage;
  auto lines = wrapText(text, xft_std_font, max_text_w);

  int block_h = static_cast<int>(lines.size()) * xft_std_font->height;
  int block_y = text_area_top + (text_area_h - block_h) / 2 + xft_std_font->ascent;

  if (index < 4) {
    string position = "Player " + to_string(index + 1);

    XftTextExtents8(display, xft_std_font,
                    (const FcChar8*)position.c_str(),
                    static_cast<int>(position.length()), &ext);

    XftDrawString8(xft_draw[index], &xft_color, xft_std_font,
                   center_x - ext.width / 2, block_y,
                   (const FcChar8*)position.c_str(),
                   static_cast<int>(position.length()));

    block_y += xft_std_font->height;
  }

  for (const auto& line : lines) {
    XftTextExtents8(display, xft_std_font,
                   (FcChar8*)line.c_str(),
                   static_cast<int>(line.length()), &ext);

    XftDrawString8(xft_draw[index], &xft_color, xft_std_font,
                   center_x - ext.width / 2, block_y,
                   (FcChar8*)line.c_str(),
                   static_cast<int>(line.length()));

    block_y += xft_std_font->height;
  }

  // Version string.
  string ver = Version::FULL;
  XftTextExtents8(display, xft_sub_font,
                  (FcChar8*)ver.c_str(),
                  static_cast<int>(ver.length()), &ext);

  XftDrawString8(xft_draw[index], &xft_color, xft_sub_font,
                 w - ext.width - 3, h - 10,
                 (FcChar8*)ver.c_str(),
                 static_cast<int>(ver.length()));

  // Copy pixmap buffer to the window.
  XCopyArea(display, pixmap_buf[index], window[index], gc[index], 0, 0, w, h, 0, 0);

  // Display it all.
  XFlush(display);
  XSync(display, False);
}

/**
 * Runs a countdown timer for a specific player window.
 *
 * @param secs Number of seconds to count down
 * @param index The index of the window/player (0-3)
 */
static void runTimer(int secs, int index)
{
  XWindowAttributes attr;
  if (!XGetWindowAttributes(display, window[index], &attr)) return;

  struct timeval start, now;
  gettimeofday(&start, nullptr);

  while (isRunning.load()) {
    gettimeofday(&now, nullptr);
    int remaining = static_cast<int>(secs - (now.tv_sec - start.tv_sec));
    if (remaining <= 0) break;

    string ct = to_string(remaining);

    {
      lock_guard<mutex> lock(timer_mtx);

      if (!XGetWindowAttributes(display, window[index], &attr)) break;

      int w = attr.width;
      int h = attr.height;

      XSetForeground(display, gc[index], WhitePixel(display, DefaultScreen(display)));
      XFillRectangle(display, pixmap_buf[index], gc[index], 0, h - xft_sub_font->height, w, h);

      XSetForeground(display, gc[index], BlackPixel(display, DefaultScreen(display)));

      XGlyphInfo ext;
      XftTextExtents8(display, xft_sub_font,
                      (FcChar8*)ct.c_str(),
                      static_cast<int>(ct.length()), &ext);

      XftDrawString8(xft_draw[index], &xft_color, xft_sub_font, 3, h - 10,
                     (FcChar8*)ct.c_str(),
                     static_cast<int>(ct.length()));

      XCopyArea(display, pixmap_buf[index], window[index], gc[index], 0, h, w, h, 0, h);
      drawWindow(index);
    }

    this_thread::sleep_for(chrono::milliseconds(250));
  }
}

/**
 * Reposition open windows to keep centered in a row.
 *
 * @param index The window index we are showing (use -1 if hiding a window).
 */
static void repositionPlayerWindows(int index = -1)
{
  int windows_open = 0;
  int w = DisplayWidth(display, DefaultScreen(display));
  int h = DisplayHeight(display, DefaultScreen(display));
  XWindowAttributes attr;

  // Count visible windows including the one about to be shown.
  for (int i = 0; i < 5; i++) {
    if (XGetWindowAttributes(display, window[i], &attr)) {
      if (attr.map_state == IsViewable || i == index) windows_open++;
    }
  }

  // Calculate total width and starting x position.
  int total_width = (windows_open * X11_WIN_WIDTH) + ((windows_open - 1) * X11_WIN_GAP);
  int current_x = (w - total_width) / 2;

  // Position each open window.
  for (int i = 0; i < 5; i++) {
    if (XGetWindowAttributes(display, window[i], &attr)) {
      if (attr.map_state == IsViewable || i == index) {
        XMoveWindow(display, window[i], current_x, (h - X11_WIN_HEIGHT) / 2);
        current_x += X11_WIN_WIDTH + X11_WIN_GAP;
      }
    }
  }
}

/**
 * Show a player window by mapping it and raising it above other windows.
 * Also sends an i3 command for games that require it.
 *
 * @param index The index of the window to show (0-4)
 */
static void showWindow(int index)
{
  cout << "Showing window: " << index << endl;

  {
    lock_guard<mutex> lock(timer_mtx);
    XMapWindow(display, window[index]);
    repositionPlayerWindows(index);
    XRaiseWindow(display, window[index]);
  }

  XSync(display, False);
}

/**
 * Hide a player window by unmapping it from the screen.
 *
 * @param index The index of the window to hide (0-4)
 */
static void hideWindow(int index)
{
  cout << "Hiding window: " << index << endl;

  {
    lock_guard<mutex> lock(timer_mtx);
    XUnmapWindow(display, window[index]);
    repositionPlayerWindows();
  }

  XSync(display, False);
}

/**
 * Run each window in a separate thread.
 * Each window has its own countdown timer.
 */
void startWindowThread(int index)
{
  if (index < 0 || index > 4 || window[index] == None) return;

  {
    lock_guard<mutex> lock(thread_mtx);

    if (windowThread[index]) {
      cout << "Thread running for window: " << index << endl;
      return;
    }

    windowThread[index] = true;
  }

  thread([index]() {
    showWindow(index);
    game->sendWindowCommands();
    runTimer(TIMER_DEFAULT, index);
    hideWindow(index);
    {
      lock_guard<mutex> lock(thread_mtx);
      windowThread[index] = false;
    }
  }).detach();
}

/**
 * Closes and cleans up all windows and X11 resources.
 */
void closeWindows()
{
  if (display != nullptr) {
    for (int i = 0; i < 5; i++) {
      // Hide all windows.
      if (window[i] != None) {
        XUnmapWindow(display, window[i]);
      }

      // Clean up pixmap resources.
      if (xft_draw[i] != nullptr) {
        XftDrawDestroy(xft_draw[i]);
        xft_draw[i] = nullptr;
      }

      // Clean up pixmap buffers.
      if (pixmap_buf[i] != None) {
        XFreePixmap(display, pixmap_buf[i]);
        pixmap_buf[i] = None;
      }

      // Clean up graphics contexts.
      if (gc[i] != nullptr) {
        XFreeGC(display, gc[i]);
        gc[i] = nullptr;
      }
    }

    // Free qr code pixmap.
    if (pixmap_qr != None) {
      XFreePixmap(display, pixmap_qr);
      pixmap_qr = None;
    }

    // Free xft color.
    if (visual != nullptr) {
      XftColorFree(display, visual, colormap, &xft_color);
    }

    // Free fonts.
    if (xft_std_font != nullptr) {
      XftFontClose(display, xft_std_font);
      xft_std_font = nullptr;
    }

    if (xft_hdr_font != nullptr) {
      XftFontClose(display, xft_hdr_font);
      xft_hdr_font = nullptr;
    }

    if (xft_sub_font != nullptr) {
      XftFontClose(display, xft_sub_font);
      xft_sub_font = nullptr;
    }

    // Remove temporary font files.
    remove((game->getTmpPath() + "/ghoulish.ttf").c_str());
    remove((game->getTmpPath() + "/roboto.ttf").c_str());

    // Destroy windows.
    for (int i = 0; i < 5; i++) {
      if (window[i] != None) {
        XDestroyWindow(display, window[i]);
        window[i] = None;
      }
    }

    // Finish up.
    XFlush(display);
    XSync(display, False);
    XCloseDisplay(display);
    display = nullptr;
  }
}

/**
 * Creates and initializes all player windows.
 */
void openWindows()
{
  if (display == nullptr) x11Init();

  // Setup screen width and height.
  int screen = DefaultScreen(display);
  int w = DisplayWidth(display, screen);
  int h = DisplayHeight(display, screen);

  // Window width and height.
  int ww = X11_WIN_WIDTH;
  int wh = X11_WIN_HEIGHT;

  // Load shared resources first.
  int rc = XpmReadFileToPixmap(
    display,
    RootWindow(display, screen),
    qrCode->getPath().c_str(),
    &pixmap_qr, NULL, NULL);

  if (rc != XpmSuccess) {
    cerr << "Failed to create pixmap: " << XpmGetErrorString(rc) << endl;
  }

  // Load TTF fonts.
  FcConfig* fc_config = FcInitLoadConfigAndFonts();
  loadFont((game->getTmpPath() + "/ghoulish.ttf").c_str(), Ghoulish_ttf, Ghoulish_ttf_len, fc_config);
  loadFont((game->getTmpPath() + "/roboto.ttf").c_str(), Roboto_ttf, Roboto_ttf_len, fc_config);
  FcConfigSetCurrent(fc_config);

  xft_std_font = XftFontOpenName(display, screen, "Ghoulish:size=31");
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

  // Setup X11 resources.
  colormap = DefaultColormap(display, screen);
  visual = DefaultVisual(display, screen);

  XftColorAllocName(
    display,
    visual,
    colormap,
    "black",
    &xft_color);

  // Create 4 player windows.
  for (int i = 0; i < 5; i++) {
    int x;
    string title;

    if (i < 4) {
      x = (w - (4 * ww + 3 + X11_WIN_GAP)) / 2 + i * (ww + X11_WIN_GAP);
      title = "Player " + to_string(i + 1);
    }
    else {
      x = (w - ww) / 2;
      title = "Spooky Scoreboard Message";
    }

    int y = (h - wh) / 2;

    window[i] = createWindow(x, y, ww, wh, title, screen);
    if (window[i] == None) {
      cerr << "Failed to create window " << i << endl;
      exit(EXIT_FAILURE);
    }

    // Create graphics context for this window.
    gc[i] = XCreateGC(display, window[i], 0, NULL);
    if (!gc[i]) {
      cerr << "Failed to create GC for window " << i << endl;
      exit(EXIT_FAILURE);
    }

    // Setup pixmap buffer for this window.
    // This will act as a double-buffer for drawing text and QR code.
    pixmap_buf[i] = XCreatePixmap(display, window[i],
      X11_WIN_WIDTH, X11_WIN_HEIGHT,
      DefaultDepth(display, screen));

    if (pixmap_buf[i] == None) {
      cerr << "Failed to create pixmap buffer window " << i << endl;
      continue;
    }

    // Create XftDraw for this window.
    xft_draw[i] = XftDrawCreate(
      display,
      pixmap_buf[i],
      visual,
      colormap);

    if (!xft_draw[i]) {
      cerr << "Failed to create XftDraw for pixmap buffer " << i << endl;
      exit(EXIT_FAILURE);
    }
  }

  // Force a sync to ensure window creation is complete.
  XSync(display, False);
}

// vim: set ts=2 sw=2 expandtab:

