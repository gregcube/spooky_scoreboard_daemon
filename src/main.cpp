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

#include <cstdint>
#include <csignal>
#include <cstring>
#include <iostream>
#include <limits.h>
#include <vector>
#include <thread>

#include <unistd.h>
#include <signal.h>
#include <sys/inotify.h>
#include <json/json.h>

#include "main.h"
#include "x11.h"
#include "Config.h"
#include "Register.h"
#include "QrScanner.h"
#include "AuditEvent.h"
#include "version.h"

using namespace std;

players playerList;

atomic<bool> isRunning{false};

unique_ptr<GameBase> game = nullptr;
unique_ptr<QrScanner> qrScanner = nullptr;
unique_ptr<QrCode> qrCode = nullptr;

shared_ptr<WebSocket> webSocket = nullptr;
shared_ptr<Player> playerHandler = nullptr;

static vector<string> savedArgv;

/**
 * Performs cleanup of all resources and threads.
 * Called both on normal exit and when handling signals.
 */
static void cleanup()
{
  // Signal all threads to stop.
  isRunning.store(false);

  // Stop QR scanner if running.
  if (qrScanner) {
    qrScanner->stop();
    qrScanner.reset();
  }

  // Cleanup X11 resources.
  closeWindows();

  // Reset pointers.
  if (game) game.reset();
  if (qrCode) qrCode.reset();
  if (playerHandler) playerHandler.reset();
  if (webSocket) webSocket.reset();
}

void restartDaemon()
{
  static atomic<bool> requested{false};
  if (requested.exchange(true)) return;

  AuditEvent::record("daemon_restart");

  thread([]() {
    // Let the websocket command handler finish sending/acking first.
    this_thread::sleep_for(chrono::milliseconds(200));

    char exePath[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (n < 0) {
      cerr << "Restart failed: cannot resolve /proc/self/exe: " << strerror(errno) << endl;
      requested.store(false);
      return;
    }
    exePath[n] = '\0';

    vector<char*> args;
    args.reserve(savedArgv.size() + 1);
    for (auto& s : savedArgv) args.push_back(s.data());
    args.push_back(nullptr);

    cout << "Restarting daemon..." << endl;
    cleanup();
    cout.flush();
    cerr.flush();

    execv(exePath, args.data());
    cerr << "Restart failed: execv: " << strerror(errno) << endl;
    _Exit(EXIT_FAILURE);
  }).detach();
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
      game->uploadScores(currentScore, game->ScoreType::High);
      lastScore = currentScore;
    }
  }
  catch (const runtime_error& e) {
    cerr << "Exception: " << e.what() << endl;
  }
}

/**
 * Process and upload last game scores.
 */
static void processLastGameScoresEvent()
{
  try {
    game->uploadScores(game->processLastGameScores(), game->ScoreType::Last);
    playerList.reset();
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

      if (strcmp(evt->name, game->getLastScoresFile().c_str()) == 0) {
        processLastGameScoresEvent();
      }
    }

    ptr += sizeof(struct inotify_event) + evt->len;
  }
}

/**
 * Sets up file watching to act on certain file changes.
 */
static void watch()
{
  int fd, wd;
  char buf[1024];

  if ((fd = inotify_init1(IN_CLOEXEC)) == -1) {
    cerr << "Failed inotify_init()." << endl;
    exit(EXIT_FAILURE);
  }

  if ((wd = inotify_add_watch(
    fd, game->getScoresPath().c_str(), IN_CLOSE_WRITE)) == -1) {

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

static void uploadHighScores()
{
  try {
    webSocket = make_shared<WebSocket>(WS_URL);
    webSocket->connect();
    Json::Value scores = game->processHighScores();
    game->uploadScores(scores, game->ScoreType::High);
  }
  catch (const runtime_error& e) {
    cerr << e.what() << endl;
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

static void registerGame(const string& code, const string& path)
{
  try {
    webSocket = make_shared<WebSocket>(WS_URL);
    webSocket->connect();
    Register(webSocket).registerMachine(code, path).get();
  }
  catch (const runtime_error& e) {
    cerr << e.what() << endl;
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

static void printSupportedGames()
{
  cout << "Spooky Scoreboard Daemon (ssbd) v" << Version::FULL << "\n";
  for (auto it = gameFactories.begin(); it != gameFactories.end(); ++it) {
    cout << "  " << it->first << ":\t" << it->second()->getGameName() << "\n";
  }
  exit(EXIT_SUCCESS);
}

static void printUsage(const char* argv0)
{
  cerr << "Spooky Scoreboard Daemon (ssbd) v" << Version::FULL << "\n";
  cerr << "Usage: " << argv0 << " [OPTIONS]" << "\n\n";
  cerr << "Options:" << endl;
  cerr << "  -g <game> Game name\n";
  cerr << "            Use -l to list supported games\n\n";
  cerr << "  -r <code> Register pinball machine\n";
  cerr << "            Use with -g <game>\n";
  cerr << "            Obtain registration code at spookyscoreboard.com\n\n";
  cerr << "  -o <path> Optionally specify path to save configuration file\n";
  cerr << "            Use with -r <code>\n\n";
  cerr << "  -u        Upload high scores and exit\n";
  cerr << "            Use with -g <game>\n\n";
  cerr << "  -l        List supported games\n\n";
  cerr << "  -h        Displays usage\n" << endl;
  exit(EXIT_SUCCESS);
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
  exit(signum);
}

int main(int argc, char** argv)
{
  savedArgv.assign(argv, argv + argc);

  string reg_code, game_name, config_path;
  bool upload = false, help = false, list = false;

  int opt;
  while ((opt = getopt(argc, argv, "hlr:uo:g:")) != -1) {
    switch (opt) {
    case 'h':
      help = true;
      break;
    case 'l':
      list = true;
      break;
    case 'r':
      reg_code = optarg;
      break;
    case 'u':
      upload = true;
      break;
    case 'o':
      config_path = optarg;
      break;
    case 'g':
      game_name = optarg;
      break;
    }
  }

  if (help) {
    printUsage(argv[0]);
  }

  if (list) {
    printSupportedGames();
  }

  atexit(cleanup);
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  if (game_name.empty()) {
    printUsage(argv[0]);
  }

  game = GameBase::create(game_name);
  if (!game) {
    cerr << "Invalid game name: " << game_name << endl;
    exit(EXIT_FAILURE);
  }

  cout << game->getGameName() << " - SSBd v" << Version::FULL << endl;

  if (!reg_code.empty()) {
    const string path = config_path.empty() ? Config::getDefaultPath() : config_path;
    registerGame(reg_code, path);
  }

  Config::load();

  if (upload) {
    uploadHighScores();
  }

  try {
    webSocket = make_shared<WebSocket>(WS_URL);
    webSocket->connect();

    // Arm before the expiry check so an early token_rotate cannot be missed.
    auto tokenRotateFuture = webSocket->waitForTokenRotate();

    // Check if auth token has expired. Do not start app ping until this passes;
    // Open will only resume ping after startPing() has set pingEnabled.
    if (webSocket->isTokenExpired().get()) {
      cout << "Token expired. Waiting for token rotation..." << endl;
      tokenRotateFuture.wait();
      cout << "Token rotation complete." << endl;
    }

    isRunning.store(true);

    // Instantiate player class.
    playerHandler = make_shared<Player>(webSocket);

    // Start QR scanner.
    qrScanner = make_unique<QrScanner>("/dev/ttyQR");
    qrScanner->start();

    // Fetch the machine's QR code.
    // The code is displayed when a user logs in,
    // which redirects to leaderboard page.
    qrCode = make_unique<QrCode>(webSocket);
    qrCode->download().get();

    // Initialize player windows.
    // Player windows are opened, but remain hidden
    // off screen until a user logs in or a message is received.
    openWindows();

    // Start ping thread.
    webSocket->startPing();

    AuditEvent::record("daemon_start");

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
  catch (const future_error& e) {
    cerr << e.what() << endl;
    exit(EXIT_FAILURE);
  }

  return 0;
}

// vim: set ts=2 sw=2 expandtab:

