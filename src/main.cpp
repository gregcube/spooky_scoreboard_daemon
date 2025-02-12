#include <cstdint>
#include <csignal>
#include <iostream>
#include <fstream>
#include <thread>

#include <unistd.h>
#include <signal.h>
#include <sys/inotify.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>
#include <fontconfig/fontconfig.h>
#include <json/json.h>

#include "main.h"
#include "CurlHandler.h"
#include "QrCode.h"
#include "QrScanner.h"
#include "GameBase.h"

char mid[MAX_UUID_LEN + 1];
char *token = nullptr;

players playerList;

static uint32_t gamesPlayed = 0;
static Display *display;
static CURLcode curlCode;

std::atomic<bool> isRunning(false);
std::unique_ptr<GameBase> game;
std::unique_ptr<CurlHandler> curlHandle;
std::unique_ptr<QrScanner> qrScanner;
std::thread ping, playerWindow;

static void cleanup()
{
  std::cout << "Cleaning up..." << std::endl;

  isRunning.store(false);

  if (qrScanner) {
    qrScanner->stop();
  }

  if (ping.joinable()) {
    ping.join();
  }

  if (display) {
    XCloseDisplay(display);
  }

  if (curlCode) {
    curl_global_cleanup();
  }

  if (token != nullptr) {
    delete[] token;
  }

  std::cout << "Exited." << std::endl;
}

static void loadMachineId()
{
  std::ifstream file("/.ssbd.json");

  if (!file.is_open()) {
    std::cerr << "Failed to open /.ssbd.json." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  Json::Value root;
  Json::Reader reader;

  if (reader.parse(file, root) == false) {
    file.close();
    std::cerr << "Failed to parse /.ssbd.json." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  file.close();

  size_t token_size = root["token"].asString().size();
  token = new char[token_size + 1];
  strncpy(token, root["token"].asString().c_str(), token_size);

  if (root["uuid"].asString().size() != MAX_UUID_LEN) {
    std::cerr << "Failed to read machine uuid." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  strncpy(mid, root["uuid"].asString().c_str(), MAX_UUID_LEN);
  mid[MAX_UUID_LEN] = '\0';
}

static void sendPing()
{
  const auto ch = std::make_unique<CurlHandler>(BASE_URL);

  while (isRunning.load()) {
    long rc;

#ifdef DEBUG
    std::cout << "Sending ping..." << std::endl;
#endif

    if ((rc = ch->post("/api/v1/ping")) != 200) {
      std::cerr << "Ping failed: " << rc << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
  }
}

static void openPlayerListWindow()
{
  uint8_t display_secs = 20;
  struct timeval start_time, current_time, update_time;
  Window window;
  Visual *visual;
  Colormap colormap;
  XftDraw *xft_draw;
  XftFont *xft_font;
  XftColor xft_color;
  Pixmap qr_pixmap;

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

  Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
  Atom wm_state_above = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
  XChangeProperty(
    display,
    window,
    wm_state,
    XA_ATOM,
    32,
    PropModeReplace,
    (unsigned char *) &wm_state_above,
    1
  );

  int rc = game->sendi3cmd(window);

  if (rc < 0) {
    std::exit(EXIT_FAILURE);
  }

  rc = XpmReadFileToPixmap(
    display, window, "/game/tmp/qrcode.xpm", &qr_pixmap, NULL, NULL);

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
  gettimeofday(&start_time, NULL);
  update_time = start_time;

  while (1) {
    if (XPending(display) > 0) {
      XEvent event;
      XNextEvent(display, &event);

      if (event.type == Expose && event.xexpose.count == 0) {
        XCopyArea(display, qr_pixmap, window, gc, 0, 0, 200, 200, ww - 210, 10);
        XClearArea(display, window, 0, 0, ww - 210, 40 * 4, 0);

        for (int i = 0, ty = 40; i < playerList.player.size(); i++, ty += 40) {
          char username[60];

          snprintf(
            username,
            sizeof(username),
            "Player %d: %s", i + 1,
            playerList.player[i].c_str());

          XftDrawString8(
            xft_draw,
            &xft_color,
            xft_font,
            10,
            ty,
            (XftChar8 *)username,
            strlen(username));
        }
      }
    }

    gettimeofday(&current_time, NULL);
    if (current_time.tv_sec - start_time.tv_sec >= display_secs) {
      break;
    }

    if (current_time.tv_sec - update_time.tv_sec >= 1) {
      char countdown[3];

      update_time = current_time;

      snprintf(
        countdown,
        sizeof(countdown),
        "%d",
        (int)(display_secs - (current_time.tv_sec - start_time.tv_sec)));

      XClearArea(display, window, 0, wh - 30, 40, 40, 1);

      XftDrawString8(
        xft_draw,
        &xft_color,
        xft_font,
        5,
        wh - 10,
        (XftChar8 *)countdown,
        strlen(countdown));
    }
  }

  XFreePixmap(display, qr_pixmap);
  XftDrawDestroy(xft_draw);
  XftColorFree(display, visual, colormap, &xft_color);
  XftFontClose(display, xft_font);
  XDestroyWindow(display, window);
  XFlush(display);

  playerList.onScreen = false;
}

void showPlayerList()
{
  if (!playerList.onScreen) {
    std::thread playerWindow(openPlayerListWindow);
    playerWindow.detach();
    playerList.onScreen = true;
  }
}

void addPlayer(const char* playerName, int position)
{
  if (playerList.numPlayers < 4 && position >= 1 && position <= 4) {
    playerList.player[position - 1] = playerName;
    ++playerList.numPlayers;
  }
}

void playerLogin(const std::vector<char>& uuid, int position)
{
  if (uuid.size() != MAX_UUID_LEN || position < 1 || position > 4) {
    return;
  }

  if (!playerList.player[position - 1].empty()) {
    showPlayerList();
    return;
  }

  std::cout << "Logging in..." << std::endl;

  Json::Value root;
  root.append(uuid.data());
  root.append(position);

  int rc = curlHandle->post("/api/v1/login", root.toStyledString());
  if (rc != 200) return;

  root.clear();

  Json::Reader reader;
  reader.parse(curlHandle->responseData, root);
  addPlayer(root["message"].asString().c_str(), position);
  showPlayerList();
}

static void uploadScores(const Json::Value& scores)
{
  std::cout << "Uploading scores..." << std::endl;

  try {
    Json::StreamWriterBuilder writerBuilder;
    writerBuilder["indentation"] = "";
    curlHandle->post("/api/v1/score", Json::writeString(writerBuilder, scores));
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}

static void processHighScoresEvent()
{
  static Json::Value lastScore;

  try {
    Json::Value currentScore = game->processHighScores();

    if (currentScore != lastScore) {
      uploadScores(currentScore);
      playerList.reset();
      lastScore = currentScore;
    }
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}

static void processEvent(char* buf, ssize_t bytes)
{
  char *ptr = buf;

  while (ptr < buf + bytes) {
    struct inotify_event *evt = (struct inotify_event *)ptr;

    if (evt->len > 0) {
#ifdef DEBUG
      std::cout << "Event: " << evt->name << std::endl;
#endif
      if (strcmp(evt->name, game->getHighScoresFile().c_str()) == 0) {
        processHighScoresEvent();
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
    ssize_t n = read(fd, buf, sizeof(buf));

    if (n < 0) {
      std::cerr << "Failed reading event" << std::endl;
    }
    else {
      std::cout << "Processing event..." << std::endl;
      processEvent(buf, n);
    }
  }
}

static void printSupportedGames()
{
  for (auto it = gameFactories.begin(); it != gameFactories.end(); ++it) {
    std::cout << "  " << it->first << ": " << it->second()->getGameName() << "\n";
  }
}

static void registerMachine(const std::string& regCode)
{
  Json::Reader reader;
  Json::Value response, code = regCode;

  std::cout << "Registering machine..." << std::endl;
  long rc = curlHandle->post("/api/v1/register", code.toStyledString());

  if (reader.parse(curlHandle->responseData, response) == false) {
    std::cerr << "Invalid JSON response from server." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  if (rc != 200) {
    std::cout << response["message"].asString() << std::endl;
    std::exit(EXIT_FAILURE);
  }

  std::ofstream file("/.ssbd.json");
  if (!file.is_open()) {
    std::cerr << "Failed to write /.ssbd.json." << std::endl;
    std::cout << "Copy/paste the below into /.ssbd.json" << std::endl;
    std::cout << response["message"] << std::endl;
    std::exit(EXIT_FAILURE);
  }

  Json::StreamWriterBuilder writer;
  file << Json::writeString(writer, response["message"]);
  file.close();

  std::cout << "File /.ssbd.json saved." << std::endl;
  std::cout << "Machine registered." << std::endl;
}

static void printUsage()
{
  std::cerr << "Spooky Scoreboard Daemon (ssbd) v" << VERSION << "\n\n";
  std::cerr << " -g <game> Game name. Use -l to list supported games\n";
  std::cerr << " -r <code> Register pinball machine\n";
  std::cerr << " -u        Upload high scores and exit\n";
  std::cerr << " -d        Forks to background (daemon mode)\n";
  std::cerr << " -l        List supported games\n";
  std::cerr << " -h        Displays usage\n" << std::endl;
}

void signalHandler(int signal)
{
  if (signal == SIGINT || signal == SIGTERM) std::exit(0);
}

int main(int argc, char** argv)
{
  int opt, reg = 0, run = 0, upload = 0;
  std::string gameName, regCode;
  pid_t pid;

  if (argc < 2) {
    std::cerr << "Missing parameters: -r <code> or -g <game>" << std::endl;
    printUsage();
    return 1;
  }

  for (optind = 1;;) {
    if ((opt = getopt(argc, argv, "r:g:ldhu")) == -1) break;

    switch (opt) {
    case 'h':
      printUsage();
      return 0;

    case 'l':
      printSupportedGames();
      return 0;

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

    case 'u':
      run = 1;
      upload = 1;
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
    curlHandle = std::make_unique<CurlHandler>(BASE_URL);
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

    if (upload) {
      Json::Value scores = game->processHighScores();
      uploadScores(scores);
      std::exit(EXIT_SUCCESS);
    }

    XInitThreads();
    setenv("DISPLAY", ":0", 0);
    if ((display = XOpenDisplay(NULL)) == NULL) {
      std::cerr << "Unable to open X display" << std::endl;
      std::exit(EXIT_FAILURE);
    }

    try {
      qrScanner = std::make_unique<QrScanner>("/dev/ttyQR");
      qrScanner->start();

      std::make_unique<QrCode>(mid)->get()->write();
      ping = std::thread(sendPing);

      watch();
    }
    catch (const std::system_error& e) {
      std::cerr << e.what() << std::endl;
      return 1;
    }
    catch (const std::runtime_error& e) {
      std::cerr << e.what() << std::endl;
      return 1;
    }
  }

  return 0;
}

// vim: set ts=2 sw=2 expandtab:

