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

#include <iostream>
#include <uuid/uuid.h>

#include "main.h"
#include "Player.h"


void Player::login(const std::vector<char>& uuid, int position)
{
  if (uuid.size() != MAX_UUID_LEN) {
    std::cerr << "Invalid UUID packet size: " << uuid.size() << std::endl;
    return;
  }

  uuid_t uuid_parsed;
  std::string uuid_str(uuid.begin(), uuid.begin() + MAX_UUID_LEN);
  if (uuid_parse(uuid_str.c_str(), uuid_parsed) != 0) {
    std::cerr << "Invalid UUID format: " << uuid_str << std::endl;
    return;
  }

  if (position < 1 || position > 4) {
    std::cerr << "Invalid player position: " << position << std::endl;
    return;
  }

  std::cout << "Player login: " << uuid_str << " at position " << position << std::endl;

  Json::Value req;
  req["path"] = "/api/v1/login";
  req["method"] = "POST";
  req["body"].append(uuid_str);
  req["body"].append(position);

  webSocket->send(req, [this](const Json::Value& response) {
    // todo: Move addPlayer() to this class.
    std::cout << response << std::endl;
  });
}

// vim: set ts=2 sw=2 expandtab:

