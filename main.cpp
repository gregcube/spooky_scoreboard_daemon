#include <cstdint>
#include <cstring>
#include <csignal>
#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>

#include <unistd.h>
#include <signal.h>
#include <sys/inotify.h>
#include <curl/curl.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/xpm.h>
#include <fontconfig/fontconfig.h>

#include "src/CurlHandler.h"
#include "src/QrCode.h"
#include "src/GameBase.h"

#define VERSION "0.0.2"
#define MAX_MACHINE_ID_LEN 36

#ifdef DEBUG
#define BASE_URL "https://hwn.local:8443"
#else
#define BASE_URL "https://scoreboard.web.net"
#endif

typedef struct {
  char code[4][5];    // Login code for each player.
  uint8_t n_players;  // Number of players (usually 1-4).
  uint8_t shown;      // Are login codes currently displayed on screen.
} login_codes_t;

static char mid[MAX_MACHINE_ID_LEN + 1];
static uint32_t gamesPlayed = 0;
static Display *display;
static CURLcode curlCode;
static login_codes_t loginCodes;

std::atomic<bool> isRunning(false);
std::unique_ptr<GameBase> game;
std::shared_ptr<CurlHandler> curlHandle;
std::thread ping, codeWindow;

static void cleanup()
{
  std::cout << "Exiting..." << std::endl;

  isRunning.store(false);

  if (ping.joinable()) {
    ping.join();
  }

  if (codeWindow.joinable()) {
    codeWindow.join();
  }

  if (display) {
    XCloseDisplay(display);
  }

  if (curlCode) {
    curl_global_cleanup();
  }
}

static void loadMachineId()
{
  std::ifstream file("/.ssbd_mid");

  if (!file.is_open()) {
    std::cerr << "Failed to load machine id" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  file.read(mid, MAX_MACHINE_ID_LEN);
  if (file.gcount() != MAX_MACHINE_ID_LEN) {
    file.close();
    std::cerr << "Failed to read machine id" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  file.close();
  mid[MAX_MACHINE_ID_LEN] = '\0';
}

static void sendPing()
{
  std::cout << "Setting up ping thread" << std::endl;
  CurlHandler ch(BASE_URL);

  while (isRunning.load()) {
#ifdef DEBUG
    std::cout << "Sending ping..." << std::endl;
#endif
    long rescode = ch.post("/spooky/ping", mid);

    if (rescode != 200) {
      std::cerr << "Failed to send ping: " << rescode << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

  std::cout << "Exiting ping thread" << std::endl;
}

static void setGamesPlayed(uint32_t *ptr)
{
  try {
    *ptr = game->getGamesPlayed();
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}

static void setLoginCodes(login_codes_t *ptr)
{
  size_t bytes = curlHandle->responseData.size();

  if (bytes % 4 != 0 || bytes > (4 * 4))
    return;

  const char *response = curlHandle->responseData.c_str();

  for (size_t i = 0; i < bytes / 4; i++) {
    memcpy(ptr->code[i], response, 4);
    ptr->code[i][4] = '\0';
    std::cout << "Code " << i << ": " << ptr->code[i] << std::endl;
    response++;
  }
}

static void showLoginCodes(void *arg)
{
  uint8_t display_secs = 20;
  struct timeval start_time, current_time, update_time;
  login_codes_t *login_code = (login_codes_t *)arg;
  Window window;
  XftDraw *xft_draw;
  XftFont *xft_font;
  XftColor xft_color;
  Pixmap qr_pixmap;
  XpmAttributes xpm_attr;

  int screen = DefaultScreen(display);

  int w = DisplayWidth(display, screen);
  int h = DisplayHeight(display, screen);

  int ww = 500;
  int wh = 230;

  window = XCreateSimpleWindow(
    display,
    RootWindow(display, screen),
    (w - ww) / 2,
    (h - wh) / 2,
    ww,
    wh,
    1,
    BlackPixel(display, screen),
    WhitePixel(display, screen)
  );

  XMapWindow(display, window);
  XRaiseWindow(display, window);

  int rc = XpmReadFileToPixmap(
    display, window, "/game/tmp/qrcode.xpm", &qr_pixmap, NULL, &xpm_attr);

  if (rc != XpmSuccess) {
    std::cerr << "Failed to create pixmap" << XpmGetErrorString(rc) << std::endl;
  }

  GC gc = XCreateGC(display, window, 0, NULL);

  FcBool result = FcConfigAppFontAddFile(
    FcConfigGetCurrent(),
    (const FcChar8 *)"/game/code/assets/fonts/Atari_Hanzel.ttf"
  );

  if (!result) {
    std::cerr << "Failed to load font" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  xft_font = XftFontOpenName(display, screen, "Hanzel:size=21");
  if (!xft_font) {
    std::cerr << "Unable to open TTF font" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  XftColorAllocName(
    display,
    DefaultVisual(display, screen),
    DefaultColormap(display, screen),
    "black",
    &xft_color
  );

  xft_draw = XftDrawCreate(
    display,
    window,
    DefaultVisual(display, screen),
    DefaultColormap(display, screen)
  );

  XSelectInput(display, window, ExposureMask);
  gettimeofday(&start_time, NULL);
  update_time = start_time;

  while (1) {
    if (XPending(display) > 0) {
      XEvent event;
      XNextEvent(display, &event);

      if (event.type == Expose && event.xexpose.count == 0) {
        XCopyArea(
          display, qr_pixmap, window, gc, 0, 0,
          xpm_attr.width, xpm_attr.height, ww - 210, 10);

        XClearArea(display, window, 0, 0, ww - 210, 40 * 4, 0);

        for (int i = 0, ty = 40; i < login_code->n_players; i++, ty += 40) {
          char code[15];

          snprintf(code, sizeof(code),
            "Player %d: %s", i + 1, login_code->code[i]);

          XftDrawString8(
            xft_draw, &xft_color, xft_font, 10, ty,
            (XftChar8 *)code, strlen(code));
        }
      }
    }

    gettimeofday(&current_time, NULL);
    if (current_time.tv_sec - start_time.tv_sec >= display_secs)
      break;

    if (current_time.tv_sec - update_time.tv_sec >= 1) {
      char countdown[3];

      update_time = current_time;
      snprintf(countdown, sizeof(countdown), "%d",
        (int)(display_secs - (current_time.tv_sec - start_time.tv_sec)));

      XClearArea(display, window, 0, wh - 30, 40, 40, 1);
      XftDrawString8(
        xft_draw, &xft_color, xft_font, 5, wh - 10,
        (XftChar8 *)countdown, strlen(countdown));
    }
  }

  XFreePixmap(display, qr_pixmap);
  XftDrawDestroy(xft_draw);

  XftColorFree(
    display,
    DefaultVisual(display, screen),
    DefaultColormap(display, screen),
    &xft_color
  );

  XftFontClose(display, xft_font);
  XDestroyWindow(display, window);
  XFlush(display);

  login_code->shown = 0;
}

static void openPlayerSpot(uint32_t nPlayers)
{
  long rc = curlHandle
    ->post("/spooky/spot", mid + std::to_string(nPlayers));

  if (rc == 200) {
    loginCodes.n_players = nPlayers;
    setLoginCodes(&loginCodes);

    if (loginCodes.shown == 0) {
      std::thread codeWindow(showLoginCodes, &loginCodes);
      codeWindow.detach();
      loginCodes.shown = 1;
    }
  }
}

static void uploadScores()
{
  std::cout << "Uploading scores" << std::endl;

  try {
    Json::StreamWriterBuilder writerBuilder;
    writerBuilder["indentation"] = "";

    Json::Value json = game->processHighScores();

    curlHandle
      ->post("/spooky/score", mid + Json::writeString(writerBuilder, json));

    setGamesPlayed(&gamesPlayed);
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}

static void processEvent(char *buf, ssize_t bytes)
{
  char *ptr = buf;
  static uint32_t lastGameCheck;

  while (ptr < buf + bytes) {
    struct inotify_event *evt = (struct inotify_event *)ptr;

    if (evt->len > 0) {
      std::cout << "Event: " << evt->name << std::endl;

      if (strcmp(evt->name, game->getAuditsFile().c_str()) == 0) {
        uint32_t gameCheck, nPlayers;
        setGamesPlayed(&gameCheck);

#ifdef DEBUG
        std::cout << "gamesPlayed: " << gamesPlayed << "\n";
        std::cout << "gameCheck: " << gameCheck << "\n";
        std::cout << "lastGameCheck: " << lastGameCheck << std::endl;
#endif

        if (gameCheck > gamesPlayed) {
          nPlayers = gameCheck - gamesPlayed;

          if (nPlayers > 4) {
            break;
          }

          if (gameCheck == lastGameCheck) {
            uploadScores();
            lastGameCheck = 0;
            break;
          }

          openPlayerSpot(nPlayers);
          lastGameCheck = gameCheck;
        }
        else {
          uploadScores();
          break;
        }
      }
    }

    ptr += sizeof(struct inotify_event) + evt->len;
  }
}

static void watch()
{
  int fd, wd;
  char buf[1024];

  if ((fd = inotify_init()) == -1) {
    std::cerr << "Failed inotify_init()" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  if ((wd = inotify_add_watch(
    fd, game->getGamePath().c_str(), IN_CLOSE_WRITE)) == -1) {

    std::cerr << "Failed inotify_add_watch()" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  while (isRunning.load()) {
    std::cout << "Waiting for action..." << std::endl;
    ssize_t bytes = read(fd, buf, sizeof(buf));

    if (bytes == -1)
      std::cerr << "Failed reading event" << std::endl;
    else {
      std::cout << "Processing event..." << std::endl;
      processEvent(buf, bytes);
    }
  }
}

static void printSupportedGames()
{
  for (auto it = gameFactories.begin(); it != gameFactories.end(); ++it) {
    std::cout << "  " << it->first << ": " << it->second()->getGameName() << "\n";
  }
}

static void registerMachine(const std::string& code) {
  long rc = curlHandle->post("/spooky/register", code);

  if (rc != 200 || curlHandle->responseData.size() != MAX_MACHINE_ID_LEN) {
    std::exit(EXIT_FAILURE);
  }

  strncpy(mid, curlHandle->responseData.c_str(), MAX_MACHINE_ID_LEN);
  std::cout << mid << std::endl;
}

static void printUsage()
{
  std::cerr << "Spooky Scoreboard Daemon (ssbd) v" << VERSION << "\n\n";
  std::cerr << " -g <game> Start game monitoring\n";
  std::cerr << " -r <code> Register pinball machine\n";
  std::cerr << " -d Forks to background (daemon mode)\n";
  std::cerr << " -l List supported games\n";
  std::cerr << " -h Displays usage\n" << std::endl;
}

void signalHandler(int signal)
{
  if (signal == SIGINT || signal == SIGTERM) std::exit(0);
}

int main(int argc, char **argv)
{
  int opt, reg = 0, run = 0;
  std::string gameName, regCode;
  pid_t pid;

  if (argc < 2) {
    std::cerr
      << "Missing parameters: -r <code> or -g <game>"
      << std::endl;

    printUsage();
    return 1;
  }

  for (optind = 1;;) {
    if ((opt = getopt(argc, argv, "r:g:ldh")) == -1) break;

    switch (opt) {
    case 'h':
      printUsage();
      break;

    case 'r':
      if (strlen(optarg) == 4) {
        reg = 1;
        regCode = optarg;
      }
      else {
        std::cerr << "Invalid code." << std::endl;
      }
      break;

    case 'g':
      if (auto it = gameFactories.find(optarg); it != gameFactories.end()) {
        run = 1;
        gameName = optarg;
      }
      else {
        std::cerr << "Invalid game.\n";
        printSupportedGames();
      }
      break;

    case 'l':
      printSupportedGames();
      break;

    case 'd':
      pid = fork();

      if (pid < 0) {
        std::cerr << "Failed to fork" << std::endl;
        std::exit(EXIT_FAILURE);
      }

      if (pid > 0) std::exit(0);
      break;
    }
  }

  if (run && reg) {
    std::cerr << "Cannot use -r and -g together" << std::endl;
    printUsage();
    return 1;
  }

  if (run || reg) {
    curlCode = curl_global_init(CURL_GLOBAL_DEFAULT);
    curlHandle = std::make_shared<CurlHandler>(BASE_URL);
  }

  std::atexit(cleanup);
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  if (reg) {
    registerMachine(regCode);
  }

  if (run && (game = GameBase::create(gameName)) != nullptr) {
    std::cout << game->getGameName() << std::endl;

    isRunning.store(true);
    loadMachineId();

    ping = std::thread(sendPing);
    ping.detach();

    XInitThreads();
    setenv("DISPLAY", ":0", 0);
    if ((display = XOpenDisplay(NULL)) == NULL) {
      std::cerr << "Unable to open X display" << std::endl;
      std::exit(EXIT_FAILURE);
    }
    else {
      std::cout << "X display opened" << std::endl;
    }

    std::make_unique<QrCode>(curlHandle)->get(mid)->write();
    setGamesPlayed(&gamesPlayed);
    watch(); 
  }

  return 0;
}
