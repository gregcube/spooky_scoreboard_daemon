#include <string>
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

#include "src/CurlHandler.h"
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
static login_codes_t loginCodes;

std::atomic<bool> isRunning(false);
std::unique_ptr<GameBase> game;
std::unique_ptr<CurlHandler> curlHandle;
std::thread ping;

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
    std::cout << "Players: " << playedDiff << std::endl;
    loginCodes.n_players = playedDiff;
    if (loginCodes.shown == 0) {
      setLoginCodes(&loginCodes);
      // TODO: Show codes in X11 window.
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

      if (strcmp(evt->name, game->highScoresFile().c_str()) == 0) {
        try {
          Json::StreamWriterBuilder writerBuilder;
          Json::Value json = game->processHighScores();

          writerBuilder["indentation"] = "";
          std::string jsonStr = Json::writeString(writerBuilder, json);
          std::cout << jsonStr << std::endl;
          curlHandle->post("/spooky/score", mid + jsonStr);

          setGamesPlayed(&gamesPlayed);
        }
        catch (const std::runtime_error& e) {
          std::cerr << "Exception: " << e.what() << std::endl;
        }
        break;
      }

      // TODO: Implement audits file in gamebase.
      if (strcmp(evt->name, "_game_audits.json") == 0) {
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
    std::exit(0);
  } 
}

int main(int argc, char **argv)
{
  int opt, reg = 0;
  std::string game_name;
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
      game_name = argv[2];
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
      isRunning.store(true);
      break;
    }
  }

  if (isRunning.load() && reg) {
    perror("Cannot use -m and -r together");
    return 1;
  }

  if (isRunning.load() || reg) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
  }

  if (isRunning.load()) {
    if (game_name.empty()) {
      std::cerr << "Missing -g <game_name>" << std::endl;
    }
    else {
      game = GameBase::create(game_name);
      curlHandle = std::make_unique<CurlHandler>("https://hwn.local:8443");

      if (game) {
        std::cout << game->name() << std::endl;

        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        loadMachineId();

        ping = std::thread(sendPing);
        ping.detach();

        setGamesPlayed(&gamesPlayed);
        watch(); 
      }
      else {
        std::cerr << "Invalid game. Supported games:<list here>\n";
      }
    }
  }

  std::cout << "Shutting down" << std::endl;
  curl_global_cleanup();
  return 0;
}
