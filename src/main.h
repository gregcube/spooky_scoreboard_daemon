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

#pragma once

#include <string>
#include <array>
#include <atomic>
#include <vector>

#include "GameBase.h"
#include "QrCode.h"
#include "WebSocketHandler.h"

#define MAX_UUID_LEN 36

#ifdef DEBUG
#define BASE_URL "https://ssb.local:8443"
#define WS_URL "ws://ssb.local:8444"
#else
#define BASE_URL "https://spookyscoreboard.com"
#define WS_URL "wss://spookyscoreboard.com:4444"
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
extern std::shared_ptr<WebSocketHandler> webSocket;
extern std::string machineId, machineUrl, token;

extern void playerLogin(const std::vector<char>& uuid, int position);

