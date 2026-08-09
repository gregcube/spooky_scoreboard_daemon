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

#include "main.h"
#include "x11.h"
#include "Player.h"

using namespace std;

// QR login payload version (must match Drupal QrLoginToken::VERSION).
static constexpr int QR_LOGIN_VERSION = 1;

void Player::login(const string& token)
{
  if (token.empty()) {
    cerr << "Empty QR login token." << endl;
    return;
  }

  if (playerList.numPlayers == 4) {
    cerr << "All player spots are occupied." << endl;
    return;
  }

  Json::Value req;
  req["path"] = "/api/v1/login";
  req["method"] = "POST";
  req["body"]["v"] = QR_LOGIN_VERSION;
  req["body"]["token"] = token;

  webSocket->enqueueMessage(req, [this](const Json::Value& response) {
    if (response["status"].asInt() != 200) {
      cerr << "Failed to login player" << endl;
      cerr << "Server returned code " << response["status"].asInt() << endl;
      return;
    }

    Json::Value user_data;
    Json::Reader().parse(response["body"].asString(), user_data);

    if (!user_data.isMember("message") ||
        !user_data["message"].isMember("username") ||
        !user_data["message"]["username"].isString() ||
        !user_data["message"].isMember("position") ||
        !user_data["message"]["position"].isInt()) {

      cerr << "Invalid login response data." << endl;
      return;
    }

    const int position = user_data["message"]["position"].asInt();
    if (position < 1 || position > 4) {
      cerr << "Invalid player position from server: " << position << endl;
      return;
    }

    // Already occupied locally (race / re-scan).
    if (!playerList.player[position - 1].empty()) {
      startWindowThread(position - 1);
      return;
    }

    cout << "Player " << position << " logging in" << endl;
    playerList.player[position - 1] = user_data["message"]["username"].asString();
    ++playerList.numPlayers;
    startWindowThread(position - 1);
  });
}

void Player::logout(int position)
{
  if (!playerList.player[position - 1].empty()) {
    cout << "Player " << position << " logging out." << endl;
    playerList.player[position - 1] = "";
    --playerList.numPlayers;
  }
}

// vim: set ts=2 sw=2 expandtab:
