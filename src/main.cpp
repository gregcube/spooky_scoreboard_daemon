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
#include <iostream>
#include <fstream>
#include <mutex>

#include <unistd.h>
#include <signal.h>
#include <sys/inotify.h>
#include <json/json.h>

#include "main.h"
#include "x11.h"
#include "Config.h"
#include "Register.h"
#include "QrScanner.h"
#include "version.h"

using namespace std;

players playerList;

atomic<bool> isRunning{false};

unique_ptr<GameBase> game = nullptr;
unique_ptr<QrScanner> qrScanner = nullptr;
unique_ptr<QrCode> qrCode = nullptr;

shared_ptr<WebSocket> webSocket = nullptr;
shared_ptr<Player> playerHandler = nullptr;

string serverMessage;

/**
 * Performs cleanup of all resources and threads.
 * This function is called both on normal exit and when handling signals.
 */
static void cleanup()
{
  cout << "Exiting." << endl << "Cleaning up..." << endl;

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
  if (webSocket) webSocket.reset();

  cout << "Cleanup complete." << endl;
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
      game->uploadScores(currentScore, game->ScoreType::High, webSocket.get());
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
    game->uploadScores(game->processLastGameScores(), game->ScoreType::Last, webSocket.get());
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
 * Prints usage information for the program.
 */
static void printUsage()
{
  cerr << "Spooky Scoreboard Daemon (ssbd) v" << Version::FULL << "\n\n";
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
    try {
      if (run) Config::load();

      webSocket = make_shared<WebSocket>(WS_URL);
      webSocket->connect();
      isRunning.store(true);
    }
    catch (const runtime_error& e) {
      cerr << e.what() << endl;
      exit(EXIT_FAILURE);
    }
  }

  atexit(cleanup);
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  if (reg) {
    cout << "Registering machine..." << endl;
    try {
      Register(webSocket).registerMachine(regCode).get();
      exit(EXIT_SUCCESS);
    }
    catch (const runtime_error& e) {
      cerr << e.what() << endl;
      exit(EXIT_FAILURE);
    }
  }

  if (run && (game = GameBase::create(gameName)) != nullptr) {
    cout << game->getGameName() << endl;

    // Upload and exit immediately if requested.
    if (upload) {
      Json::Value scores = game->processHighScores();
      game->uploadScores(scores, game->ScoreType::High, webSocket.get());
      exit(EXIT_SUCCESS);
    }

    try {
      // Instantiate player class.
      playerHandler = make_shared<Player>(webSocket);

      // Start QR scanner.
      qrScanner = make_unique<QrScanner>("/dev/ttyQR");
      qrScanner->start();

      // Fetch the machine's QR code and URL.
      // The code and URL are displayed when a user logs in,
      // which redirects to leaderboard page.
      qrCode = make_unique<QrCode>(webSocket);
      qrCode->download().get();
      game->setUrl(webSocket.get());

      // Initialize player windows.
      // Player windows are opened, but remain hidden
      // off screen until a user logs in.
      openWindows();

      // Start ping thread.
      webSocket->startPing();

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

