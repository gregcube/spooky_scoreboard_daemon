// Spooky Scoreboard Daemon
// https://spookyscoreboard.com

#include <cstdint>
#include <csignal>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>

#include <unistd.h>
#include <signal.h>
#include <sys/inotify.h>
#include <json/json.h>

#include "main.h"
#include "x11.h"
#include "CurlHandler.h"
#include "QrScanner.h"

#define TIMER_DEFAULT 15

using namespace std;

players playerList;
static CURLcode curlCode;

atomic<bool> isRunning(false);
vector<bool> playerThread(4, false);

unique_ptr<GameBase> game = nullptr;
unique_ptr<CurlHandler> curlHandle = nullptr;
unique_ptr<QrScanner> qrScanner = nullptr;
unique_ptr<QrCode> qrCode = nullptr;

string machineId, machineUrl, token;
thread ping;
mutex mtx;

/**
 * Performs cleanup of all resources and threads.
 * This function is called both on normal exit and when handling signals.
 */
static void cleanup()
{
  cout << "Cleaning up..." << endl;

  // Signal all threads to stop.
  isRunning.store(false);

  // Stop QR scanner if running.
  if (qrScanner) {
    cout << "Stopping QR scanner..." << endl;
    qrScanner->stop();
    qrScanner.reset();
  }

  // Wait for ping thread to complete.
  if (ping.joinable()) {
    cout << "Waiting for ping thread..." << endl;
    ping.join();
  }

  // Cleanup X11 resources.
  closePlayerWindows();

  // Clean up curl resources.
  if (curlCode) {
    cout << "Cleaning up curl..." << endl;
    curl_global_cleanup();
    curlHandle.reset();
  }

  // Reset pointers.
  if (game) game.reset();
  if (qrCode) qrCode.reset();

  cout << "Cleanup complete." << endl;
}

/**
 * Loads the machine ID and token from the configuration file.
 * The file is expected to be at /.ssbd.json and contain a valid UUID and token.
 */
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

/**
 * Sends a ping to the server to indicate the machine is online.
 * This is called periodically by the ping thread.
 *
 * @param ch The CurlHandler instance to use for the request
 */
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

/**
 * Thread function that periodically sends pings to the server.
 * Runs every 10 seconds while the program is running.
 */
static void pingThread()
{
  const auto ch = make_unique<CurlHandler>(BASE_URL);
  while (isRunning.load()) {
    sendPing(ch);
    this_thread::sleep_for(chrono::seconds(10));
  }
}

/**
 * Run each player window/ countdown in a separate thread.
 *
 * @param index Player position index. Should be 0-3.
 */
static void startPlayerThread(int index)
{
  {
    lock_guard<mutex> lock(mtx);

    if (playerThread[index]) {
      cout << "Thread for player " << index << " is running." << endl;
      return;
    }

    playerThread[index] = true;
  }

  thread([index]() {
    showPlayerWindow(index);
    runTimer(TIMER_DEFAULT, index);
    hidePlayerWindow(index);
    {
      lock_guard<mutex> lock(mtx);
      playerThread[index] = false;
    }
  }).detach();
}

/**
 * Adds a player to the player list at the specified position.
 *
 * @param playerName The name of the player to add
 * @param position The position (1-4) where the player should be added
 */
void addPlayer(const char* playerName, int position)
{
  if (playerList.numPlayers < 4 && position >= 1 && position <= 4) {
    playerList.player[position - 1] = playerName;
    startPlayerThread(position - 1);
    ++playerList.numPlayers;
  }
}

/**
 * Handles player login by sending the UUID to the server and updating the player list.
 *
 * @param uuid The player's UUID
 * @param position The position (1-4) where the player should be logged in
 */
void playerLogin(const vector<char>& uuid, int position)
{
  if (uuid.size() != MAX_UUID_LEN || position < 1 || position > 4) {
    return;
  }

  // Show player window if position is already occupied.
  if (!playerList.player[position - 1].empty()) {
    cout << "Player " << position << " already logged in, showing window..." << endl;
    startPlayerThread(position - 1);
    return;
  }

  cout << "Logging in player " << position << "..." << endl;

  Json::Value root;
  root.append(uuid.data());
  root.append(position);

  long rc = curlHandle->post("/api/v1/login", root.toStyledString());
  if (rc != 200) {
    cerr << "Login failed with code: " << rc << endl;
    return;
  }

  root.clear();

  Json::Reader reader;
  reader.parse(curlHandle->responseData, root);
  addPlayer(root["message"].asString().c_str(), position);
}

/**
 * Uploads high scores to the server.
 *
 * @param scores The JSON object containing the scores to upload
 */
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

/**
 * Processes high scores from the game and uploads them if they've changed.
 * Maintains the last uploaded score to avoid duplicate uploads.
 */
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

/**
 * Processes inotify events for file changes.
 * Handles high score file changes and triggers appropriate actions.
 *
 * @param buf The buffer containing the inotify events
 * @param bytes The number of bytes in the buffer
 */
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

/**
 * Sets up file watching for the game directory.
 */
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

/**
 * Prints a list of all supported games to the console.
 */
static void printSupportedGames()
{
  for (auto it = gameFactories.begin(); it != gameFactories.end(); ++it) {
    cout << "  " << it->first << ":\t" << it->second()->getGameName() << "\n";
  }
}

/**
 * Retrieves the machine's URL from the server.
 */
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

/**
 * Registers a new machine with the server using the provided registration code.
 * Creates the configuration file with the server's response.
 *
 * @param regCode The registration code to use
 */
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

/**
 * Prints usage information for the program.
 */
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

/**
 * Signal handler for SIGINT and SIGTERM.
 * Ensures clean shutdown when the program is interrupted.
 *
 * @param signum The signal number that was received
 */
static void signalHandler(int signum)
{
  cout << "Signal " << signum << " received." << endl;
  cleanup();
  exit(signum);
}

/**
 * Main entry point for the Spooky Scoreboard Daemon.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return Exit status code
 */
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

    // Upload and exit immediately if requested.
    if (upload) {
      Json::Value scores = game->processHighScores();
      uploadScores(scores);
      exit(EXIT_SUCCESS);
    }

    try {
      // Start QR scanner.
      qrScanner = make_unique<QrScanner>("/dev/ttyQR");
      qrScanner->start();

      // Fetch the machine's QR code and URL.
      // The code and URL are displayed when a user logs in,
      // which redirects to leaderboard page.
      qrCode = make_unique<QrCode>();
      qrCode->get()->write();
      getMachineUrl();

      // Initialize player windows and ping thread.
      // Player windows are opened, but remain hidden
      // off screen until a user logs in.
      openPlayerWindows();
      ping = thread(pingThread);

      // Start main loop and watch for action.
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

