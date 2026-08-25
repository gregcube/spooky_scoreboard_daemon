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

#include "GameBase.h"
#include "QrCode.h"
#include "WebSocket.h"
#include "Player.h"

#ifndef SSB_ENV
#error "SSB_ENV is not defined. Configure with -DSSB_ENV=local|stage|live"
#endif
#ifndef SSB_WS_URL
#error "SSB_WS_URL is not defined. Configure with -DSSB_ENV=local|stage|live"
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
extern std::shared_ptr<WebSocket> webSocket;
extern std::shared_ptr<Player> playerHandler;

void restartDaemon();

// vim: set ts=2 sw=2 expandtab:

