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
#include <json/json.h>

#include "WebSocket.h"

class Register
{
public:
  Register(const std::shared_ptr<WebSocket>& ws) : webSocket(ws) {};
  void registerMachine(const std::string& regcode);

private:
  const std::shared_ptr<WebSocket> webSocket;
};

// vim: set ts=2 sw=2 expandtab:

