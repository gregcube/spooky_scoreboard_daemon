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
  int center_x = X11_WIN_WIDTH / 2;

  // Create a white rectangle for centered content
  // and the version number in bottom right corner.
  XSetForeground(display, gc[index], WhitePixel(display, screen));
  XFillRectangle(display, pixmap_buf[index], gc[index], 0, 0, X11_WIN_WIDTH, X11_WIN_HEIGHT - 30);
  XFillRectangle(display, pixmap_buf[index], gc[index], center_x, X11_WIN_HEIGHT - 30, center_x, 30);

  // Set foreground for black text.
  XSetForeground(display, gc[index], BlackPixel(display, screen));

  // Draw "Spooky" text.
  XftDrawString8(xft_draw[index], &xft_color, xft_hdr_font,
    center_x - 90, 50, (const FcChar8*)"Spooky", 6);

  // Draw "Scoreboard" text.
  XftDrawString8(xft_draw[index], &xft_color, xft_hdr_font,
    center_x - 140, 90, (const FcChar8*)"Scoreboard", 10);

  // Copy QR code pixmap to pixmap buffer.
  XCopyArea(display, pixmap_qr, pixmap_buf[index], gc[index], 0, 0, 145, 145,
    (X11_WIN_WIDTH - 145) / 2, (X11_WIN_HEIGHT - 200) / 2);

  // Draw version number in bottom right corner.
  XftDrawString8(xft_draw[index], &xft_color, xft_sub_font,
    X11_WIN_WIDTH - 70, X11_WIN_HEIGHT - 10,
    (const FcChar8*)Version::FULL, strlen(Version::FULL));

  if (index < 4) {
    // Draw "Player <num>" text.
    string position = "Player " + to_string(index + 1);
    XftDrawString8(xft_draw[index], &xft_color, xft_std_font,
      center_x - static_cast<int>(position.length() * 10), 370,
      (const FcChar8*)position.c_str(), static_cast<int>(position.length()));

    // Draw player's online name.
    const string& playerName = playerList.player[index];
    XftDrawString8(xft_draw[index], &xft_color, xft_std_font,
      center_x - static_cast<int>(static_cast<double>(playerName.length()) * 10.5), 410,
      (const FcChar8*)playerName.c_str(), static_cast<int>(playerName.length()));
  }
  else {
    // Draw server message.
    XftDrawString8(xft_draw[index], &xft_color, xft_std_font,
      center_x - static_cast<int>(serverMessage.length() * 10), 370,
      (const FcChar8*)serverMessage.c_str(), static_cast<int>(serverMessage.length()));
  }

  // Copy pixmap buffer to the window.
  XCopyArea(display, pixmap_buf[index], window[index], gc[index], 0, 0, X11_WIN_WIDTH, X11_WIN_HEIGHT, 0, 0);

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
  struct timeval start_time, current_time, update_time;
  gettimeofday(&start_time, NULL);
  update_time = start_time;

  while (isRunning.load()) {
    // Check and exit if timer has expired.
    gettimeofday(&current_time, NULL);
    int remaining = static_cast<int>(secs - (current_time.tv_sec - start_time.tv_sec));
    if (remaining <= 0) break;

    // Timer hasn't expired.
    // Setup and display count down timer.
    if (remaining > 0) {
      string ct = to_string(remaining);

      {
        // Aquire lock to ensure thread-safe access to X11 resources.
        lock_guard<mutex> lock(timer_mtx);

        // Create a white rectangle area for count down timer.
        XSetForeground(display, gc[index], WhitePixel(display, DefaultScreen(display)));
        XFillRectangle(display, pixmap_buf[index], gc[index], 0, X11_WIN_HEIGHT - 60, X11_WIN_WIDTH / 2, 60);

        // Set foreground to black.
        XSetForeground(display, gc[index], BlackPixel(display, DefaultScreen(display)));

        // Draw count down timer in bottom, left corner.
        XftDrawString8(xft_draw[index], &xft_color, xft_sub_font,
          5, X11_WIN_HEIGHT - 10, (FcChar8*)ct.c_str(), static_cast<int>(ct.length()));

        // Copy pixmap buffer to window.
        XCopyArea(display, pixmap_buf[index], window[index], gc[index],
          0, X11_WIN_HEIGHT - 60, X11_WIN_WIDTH / 2, 60, 0, X11_WIN_HEIGHT - 60);

        // Update window.
        drawWindow(index);
      }

      update_time = current_time;
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
    if (game->sendWindowCommands() == 0) {
      showWindow(index);
      runTimer(TIMER_DEFAULT, index);
      hideWindow(index);
      {
        lock_guard<mutex> lock(thread_mtx);
        windowThread[index] = false;
      }
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

