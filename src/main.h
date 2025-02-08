#ifndef _MAIN_H
#define _MAIN_H

#include <string>
#include <array>
#include <atomic>

#define VERSION "0.0.5-1"
#define MAX_UUID_LEN 36

#ifdef DEBUG
#define BASE_URL "https://ssb.local:8443"
#else
#define BASE_URL "https://spookyscoreboard.com"
#endif

typedef struct {
  std::array<std::string, 4> player;
  uint8_t numPlayers;
  bool onScreen;

  void reset() {
    for (auto &p : player) p.clear();
    numPlayers = 0;
    onScreen = false;
  }
} players;

extern players playerList;
extern char mid[MAX_UUID_LEN + 1];
extern std::atomic<bool> isRunning;

extern void addPlayer(const char *playerName, int position);
extern void showPlayerList();

#endif
