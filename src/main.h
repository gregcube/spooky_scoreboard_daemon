#ifndef _MAIN_H
#define _MAIN_H

#include <string>
#include <array>
#include <atomic>
#include <vector>

#include "GameBase.h"

#define VERSION "0.0.7-1"
#define MAX_UUID_LEN 36

#ifdef DEBUG
#define BASE_URL "https://ssb.local:8443"
#else
#define BASE_URL "https://spookyscoreboard.com"
#endif

struct players {
  std::array<std::string, 4> player{};
  uint8_t numPlayers{0};
  bool onScreen{false};

  void reset() {
    std::fill(player.begin(), player.end(), "");
    numPlayers = 0;
    onScreen = false;
  }
};

extern players playerList;
extern std::atomic<bool> isRunning;
extern std::unique_ptr<GameBase> game;
extern std::string machineId, machineUrl, token;

extern void playerLogin(const std::vector<char>& uuid, int position);

#endif
