// Spooky Scoreboard Daemon
// https://spookyscoreboard.com

#include <cstdint>
#include <csignal>
#include <iostream>
#include <fstream>
#include <thread>

#include <unistd.h>
#include <signal.h>
#include <sys/inotify.h>
#include <json/json.h>

#include "main.h"
#include "x11.h"
#include "CurlHandler.h"
#include "QrCode.h"
#include "QrScanner.h"

using namespace std;

players playerList;
static CURLcode curlCode;

atomic<bool> isRunning(false);
unique_ptr<GameBase> game = nullptr;
unique_ptr<CurlHandler> curlHandle = nullptr;
unique_ptr<QrScanner> qrScanner = nullptr;
thread ping, playerWindow;
string machineId, machineUrl, token;

static void cleanup()
{
  cout << "Cleaning up..." << endl;

  isRunning.store(false);

  if (qrScanner) {
    qrScanner->stop();
  }

  if (ping.joinable()) {
    ping.join();
  }

  if (playerWindow.joinable()) {
    playerWindow.join();
  }

  if (curlCode) {
    curl_global_cleanup();
  }

  cout << "Exited." << endl;
}

static void loadMachineId()
{
  ifstream file("/.ssbd.json");

  if (!file.is_open()) {
    cerr << "Failed to open /.ssbd.json." << endl;
    exit(EXIT_FAILURE);
  }

  Json::Value root;
  Json::Reader reader;

  if (reader.parse(file, root) == false) {
    file.close();
    cerr << "Failed to parse /.ssbd.json." << endl;
    exit(EXIT_FAILURE);
  }

  file.close();

  if (root["uuid"].asString().size() != MAX_UUID_LEN) {
    cerr << "Failed to read machine uuid." << endl;
    exit(EXIT_FAILURE);
  }

  token = root["token"].asString();
  machineId = root["uuid"].asString();
}

// todo: Exit after failing 10 or more pings(?)
static void sendPing(const unique_ptr<CurlHandler>& ch)
{
#ifdef DEBUG
  cout << "Sending ping..." << endl;
#endif
  long rc;
  if ((rc = ch->post("/api/v1/ping")) != 200) {
    cerr << "Ping failed: " << rc << endl;
  }
}

static void playerWindowThread()
{
  openPlayerListWindow();

  while (isRunning.load()) {
    if (playerList.onScreen) {
      showPlayerListWindow();
      runTimer(20);
      hidePlayerListWindow();
      playerList.onScreen = false;
    }
    else {
      this_thread::sleep_for(chrono::milliseconds(50));
    }
  }

  closePlayerListWindow();
}

static void pingThread()
{
  const auto ch = make_unique<CurlHandler>(BASE_URL);
  while (isRunning.load()) {
    sendPing(ch);
    this_thread::sleep_for(chrono::seconds(10));
  }
}

void addPlayer(const char* playerName, int position)
{
  if (playerList.numPlayers < 4 && position >= 1 && position <= 4) {
    playerList.player[position - 1] = playerName;
    ++playerList.numPlayers;
  }
}

void playerLogin(const vector<char>& uuid, int position)
{
  if (uuid.size() != MAX_UUID_LEN || position < 1 || position > 4) {
    return;
  }

  if (!playerList.player[position - 1].empty()) {
    playerList.onScreen = true;
    return;
  }

  cout << "Logging in..." << endl;

  Json::Value root;
  root.append(uuid.data());
  root.append(position);

  int rc = curlHandle->post("/api/v1/login", root.toStyledString());
  if (rc != 200) return;

  root.clear();

  Json::Reader reader;
  reader.parse(curlHandle->responseData, root);
  addPlayer(root["message"].asString().c_str(), position);
  playerList.onScreen = true;
}

static void uploadScores(const Json::Value& scores)
{
  cout << "Uploading scores..." << endl;

  try {
    Json::StreamWriterBuilder writerBuilder;
    writerBuilder["indentation"] = "";
    curlHandle->post("/api/v1/score", Json::writeString(writerBuilder, scores));
  }
  catch (const runtime_error& e) {
    cerr << "Exception: " << e.what() << endl;
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
  catch (const runtime_error& e) {
    cerr << "Exception: " << e.what() << endl;
  }
}

static void processEvent(char* buf, ssize_t bytes)
{
  char* ptr = buf;

  while (ptr < buf + bytes) {
    struct inotify_event* evt = (struct inotify_event*)ptr;

    if (evt->len > 0) {
#ifdef DEBUG
      cout << "Event: " << evt->name << endl;
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
    cerr << "Failed inotify_init()." << endl;
    exit(EXIT_FAILURE);
  }

  if ((wd = inotify_add_watch(
    fd, game->getGamePath().c_str(), IN_CLOSE_WRITE)) == -1) {

    cerr << "Failed inotify_add_watch()." << endl;
    exit(EXIT_FAILURE);
  }

  while (isRunning.load()) {
    cout << "Waiting for action..." << endl;
    ssize_t n = read(fd, buf, sizeof(buf));

    if (n < 0) {
      cerr << "Failed reading event." << endl;
    }
    else {
      cout << "Processing event..." << endl;
      processEvent(buf, n);
    }
  }
}

static void printSupportedGames()
{
  for (auto it = gameFactories.begin(); it != gameFactories.end(); ++it) {
    cout << "  " << it->first << ": " << it->second()->getGameName() << "\n";
  }
}

static void getMachineUrl()
{
  Json::Reader reader;
  Json::Value response;

  cout << "Retrieving machine URL..." << endl;
  long rc = curlHandle->get("/api/v1/url");

  if (reader.parse(curlHandle->responseData, response) == false) {
    cerr << "Invalid JSON response from server." << endl;
    exit(EXIT_FAILURE);
  }

  if (rc != 200) {
    cout << response["message"].asString() << endl;
    exit(EXIT_FAILURE);
  }

  size_t len = response["message"].asString().size();
  machineUrl = response["message"].asString().substr(8, len - 8);
}

static void registerMachine(const string& regCode)
{
  Json::Reader reader;
  Json::Value response, code = regCode;

  cout << "Registering machine..." << endl;
  long rc = curlHandle->post("/api/v1/register", code.toStyledString());

  if (reader.parse(curlHandle->responseData, response) == false) {
    cerr << "Invalid JSON response from server." << endl;
    exit(EXIT_FAILURE);
  }

  if (rc != 200) {
    cout << response["message"].asString() << endl;
    exit(EXIT_FAILURE);
  }

  ofstream file("/.ssbd.json");
  if (!file.is_open()) {
    cerr << "Failed to write /.ssbd.json." << endl;
    cout << "Copy/paste the below into /.ssbd.json" << endl;
    cout << response["message"] << endl;
    exit(EXIT_FAILURE);
  }

  Json::StreamWriterBuilder writer;
  file << Json::writeString(writer, response["message"]);
  file.close();

  cout << "File /.ssbd.json saved." << endl;
  cout << "Machine registered." << endl;
}

static void printUsage()
{
  cerr << "Spooky Scoreboard Daemon (ssbd) v" << VERSION << "\n\n";
  cerr << " -g <game> Game name.\n";
  cerr << "           Use -l to list supported games.\n\n";
  cerr << " -r <code> Register pinball machine.\n";
  cerr << "           Obtain registration code at spookyscoreboard.com.\n\n";
  cerr << " -u        Upload high scores and exit.\n\n";
  cerr << " -d        Fork to background (daemon mode).\n\n";
  cerr << " -l        List supported games.\n\n";
  cerr << " -h        Displays usage.\n" << endl;
}

void signalHandler(int signal)
{
  if (signal == SIGINT || signal == SIGTERM) exit(0);
}

int main(int argc, char** argv)
{
  int opt, reg = 0, run = 0, upload = 0;
  string gameName, regCode;
  pid_t pid;

  if (argc < 2) {
    cerr << "Missing parameters: -r <code> or -g <game>" << endl;
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
        cerr << "Invalid code." << endl;
      }
      break;

    case 'g':
      if (auto it = gameFactories.find(optarg); it != gameFactories.end()) {
        run = 1;
        gameName = optarg;
      }
      else {
        cerr << "Invalid game.\n";
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
        cerr << "Failed to fork" << endl;
        exit(EXIT_FAILURE);
      }

      if (pid > 0) exit(0);
      break;
    }
  }

  if (run && reg) {
    cerr << "Cannot use -r and -g together" << endl;
    printUsage();
    return 1;
  }

  if (run || reg) {
    curlCode = curl_global_init(CURL_GLOBAL_DEFAULT);
    curlHandle = make_unique<CurlHandler>(BASE_URL);
  }

  atexit(cleanup);
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  if (reg) {
    registerMachine(regCode);
  }

  if (run && (game = GameBase::create(gameName)) != nullptr) {
    cout << game->getGameName() << endl;

    isRunning.store(true);
    loadMachineId();

    if (upload) {
      Json::Value scores = game->processHighScores();
      uploadScores(scores);
      exit(EXIT_SUCCESS);
    }

    try {
      qrScanner = make_unique<QrScanner>("/dev/ttyQR");
      qrScanner->start();

      make_unique<QrCode>()->get()->write();

      playerWindow = thread(playerWindowThread);
      ping = thread(pingThread);

      getMachineUrl();
      watch();
    }
    catch (const system_error& e) {
      cerr << e.what() << endl;
      exit(EXIT_FAILURE);
    }
    catch (const runtime_error& e) {
      cerr << e.what() << endl;
      exit(EXIT_FAILURE);
    }
  }

  return 0;
}

// vim: set ts=2 sw=2 expandtab:

