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

typedef struct {
  char code[4][5];    // Login code for each player.
  uint8_t n_players;  // Number of players (usually 1-4).
  uint8_t shown;      // Are login codes currently displayed on screen.
} login_codes_t;

static char mid[MAX_MACHINE_ID_LEN + 1];
static uint32_t gamesPlayed = 0;
static Display *display;
static login_codes_t loginCodes;

std::atomic<bool> isRunning(false);
std::unique_ptr<GameBase> game;
std::shared_ptr<CurlHandler> curlHandle;
std::thread ping, codeWindow;

static void loadMachineId()
{
  std::ifstream file("/.ssbd_mid");

  if (!file.is_open()) {
    std::cerr << "Failed to load machine id" << std::endl;
  }

  file.read(mid, MAX_MACHINE_ID_LEN);
  if (file.gcount() != MAX_MACHINE_ID_LEN) {
    std::cerr << "Failed to read machine id" << std::endl;
    file.close();
  }

  file.close();
  mid[MAX_MACHINE_ID_LEN] = '\0';
}

static void sendPing()
{
  std::cout << "Setting up ping thread" << std::endl;
  CurlHandler ch("https://hwn.local:8443");

  while (isRunning.load()) {
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
    std::cout << ptr << ": " << *ptr << std::endl;
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
    fprintf(stderr, "Failed to create pixmap: %s\n", XpmGetErrorString(rc));
    //cleanup(1);
  }

  GC gc = XCreateGC(display, window, 0, NULL);

  FcBool result = FcConfigAppFontAddFile(
    FcConfigGetCurrent(),
    (const FcChar8 *)"/game/code/assets/fonts/Atari_Hanzel.ttf"
  );

  if (!result) {
    fprintf(stderr, "FontConfig: Failed to load font.\n");
    //cleanup(1);
  }

  xft_font = XftFontOpenName(display, screen, "Hanzel:size=21");
  if (!xft_font) {
    fprintf(stderr, "Xft: Unable to open TTF font.\n");
    //cleanup(1);
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

static void setPlayerSpot()
{
  static uint32_t lastPlayed = 0;
  uint32_t nowPlayed;
  uint32_t playedDiff;

  setGamesPlayed(&nowPlayed);
  playedDiff = nowPlayed - gamesPlayed;

  if (playedDiff == 0 || playedDiff > 4 || playedDiff == lastPlayed) {
    return;
  }

  std::string post = mid + std::to_string(playedDiff);
  long rc = curlHandle->post("/spooky/spot", post);

  if (rc == 200) {
    loginCodes.n_players = playedDiff;
    setLoginCodes(&loginCodes);

    if (loginCodes.shown == 0) {
      std::thread codeWindow(showLoginCodes, &loginCodes);
      codeWindow.detach();
      loginCodes.shown = 1;
    }
  }

  lastPlayed = nowPlayed;
}

static void processEvent(char *buf, ssize_t bytes)
{
  char *ptr = buf;
  while (ptr < buf + bytes) {
    struct inotify_event *evt = (struct inotify_event *)ptr;

    if (evt->len > 0) {
      std::cout << "Event: " << evt->name << std::endl;

      if (strcmp(evt->name, game->getScoresFile().c_str()) == 0) {
        try {
          Json::StreamWriterBuilder writerBuilder;
          Json::Value json = game->processHighScores();

          writerBuilder["indentation"] = "";
          std::string jsonStr = Json::writeString(writerBuilder, json);
          curlHandle->post("/spooky/score", mid + jsonStr);

          setGamesPlayed(&gamesPlayed);
        }
        catch (const std::runtime_error& e) {
          std::cerr << "Exception: " << e.what() << std::endl;
        }
        break;
      }

      if (strcmp(evt->name, game->getAuditsFile().c_str()) == 0) {
        setPlayerSpot();
        break;
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
    perror("Failed inotify_init()");
    //cleanup(1);
  }

  if ((wd = inotify_add_watch(fd, game->getGamePath().c_str(), IN_CLOSE_WRITE)) == -1) {
    perror("Failed inotify_add_watch()");
    //cleanup(1);
  }

  while (isRunning.load()) {
    std::cout << "Watching for action" << std::endl;
    ssize_t bytes = read(fd, buf, sizeof(buf));

    if (bytes == -1)
      std::cerr << "Failed reading event" << std::endl;
    else {
      std::cout << "Processing event..." << std::endl;
      processEvent(buf, bytes);
    }
  }
}

static int registerMachine(const char *code)
{
  long rc = curlHandle->post("/spooky/register", code);

  if (rc != 200 || curlHandle->responseData.size() != MAX_MACHINE_ID_LEN) {
    return 1;
  }

  strncpy(mid, curlHandle->responseData.c_str(), MAX_MACHINE_ID_LEN);
  std::cout << mid << std::endl;

  return 0;
}

static void print_usage()
{
  std::cerr << "Spooky Scoreboard Daemon (ssbd) v" << VERSION << "\n\n";
  std::cerr << " -g <game> Specify the game" << std::endl;
  std::cerr << " -m Monitor highscores & send to server" << std::endl;
  std::cerr << " -d Run in background (daemon mode)" << std::endl;
  std::cerr << " -r <code> Register your pinball machine" << std::endl;
  std::cerr << " -h Displays this" << std::endl;
}

void signalHandler(int signal)
{
  if (signal == SIGINT || signal == SIGTERM) {
    std::cout << "Cleaning up..." << std::endl;

    isRunning.store(false);
    curl_global_cleanup();

    if (display) {
      XCloseDisplay(display);
    }

    std::exit(0);
  } 
}

int main(int argc, char **argv)
{
  int opt, reg = 0, run = 0;
  std::string gameName;
  pid_t pid;

  if (argc < 2) {
    std::cerr
      << "Missing parameters: -r <reg code> or -m is required, -g <game name> too!"
      << std::endl;

    print_usage();
    return 1;
  }

  for (optind = 1;;) {
    if ((opt = getopt(argc, argv, "r:g:mdh")) == -1) break;

    switch (opt) {
    case 'h':
      print_usage();
      break;

    case 'r':
      if (argv[2] != NULL && strlen(argv[2]) == 4) reg = 1;
      break;

    case 'g':
      gameName = argv[2];
      break;

    case 'd':
      pid = fork();

      if (pid < 0) {
        perror("Failed to fork");
        exit(1);
      }

      if (pid > 0) exit(0);
      break;

    case 'm':
      run = 1;
      break;
    }
  }

  if (run && reg) {
    std::cerr << "Cannot use -m and -r together" << std::endl;
    return 1;
  }

  if (run || reg) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curlHandle = std::make_shared<CurlHandler>("https://hwn.local:8443");
  }

  if (reg) {
    registerMachine(argv[2]);
  }

  if (run) {
    if (gameName.empty()) {
      std::cerr << "Missing -g <gameName>" << std::endl;
      return 1;
    }
    else {
      game = GameBase::create(gameName);
      if (game) {
        std::cout << game->getGameName() << std::endl;

        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        isRunning.store(true);
        loadMachineId();

        ping = std::thread(sendPing);
        ping.detach();

        XInitThreads();
        setenv("DISPLAY", ":0", 0);
        if ((display = XOpenDisplay(NULL)) == NULL) {
          std::cerr << "Unable to open X display" << std::endl;
          return 1;
        }
        else {
          std::cout << "X display opened" << std::endl;
        }

        std::make_unique<QrCode>(curlHandle)->get(mid)->write();
        setGamesPlayed(&gamesPlayed);
        watch(); 
      }
      else {
        std::cerr << "Invalid game. Supported games:<list here>\n";
        return 1;
      }
    }
  }

  curl_global_cleanup();
  return 0;
}
