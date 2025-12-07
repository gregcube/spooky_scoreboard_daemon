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

#include "WebSocketHandler.h"

class Player {
public:
  Player(const std::shared_ptr<WebSocketHandler>& ws) : webSocket(ws) {};

  void login(const std::vector<char>& uuid, int position);

private:
  std::shared_ptr<WebSocketHandler> webSocket;
};


// vim: set ts=2 sw=2 expandtab:

