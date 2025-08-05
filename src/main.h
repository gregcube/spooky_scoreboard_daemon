#ifndef _MAIN_H
#define _MAIN_H

#include <string>
#include <array>
#include <atomic>
#include <vector>

#include "GameBase.h"
#include "QrCode.h"

#define VERSION "0.0.9-1"
#define MAX_UUID_LEN 36

#ifdef DEBUG
#define BASE_URL "https://ssb.local:8443"
#else
#define BASE_URL "https://spookyscoreboard.com"
#endif

struct players {
  uint8_t numPlayers{0};
  std::array<std::string, 4> player{};

  void reset() {
    numPlayers = 0;
    std::fill(player.begin(), player.end(), "");
  }
};

extern players playerList;
extern std::atomic<bool> isRunning;
extern std::unique_ptr<GameBase> game;
extern std::unique_ptr<QrCode> qrCode;
extern std::string machineId, machineUrl, token;

extern void playerLogin(const std::vector<char>& uuid, int position);

#endif
