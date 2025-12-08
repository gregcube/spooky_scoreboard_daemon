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
#include "x11.h"
#include "Player.h"

using namespace std;

void Player::login(const vector<char>& uuid, int position)
{
  if (uuid.size() != MAX_UUID_LEN) {
    cerr << "Invalid UUID packet size: " << uuid.size() << endl;
    return;
  }

  uuid_t uuid_parsed;
  string uuid_str(uuid.begin(), uuid.begin() + MAX_UUID_LEN);
  if (uuid_parse(uuid_str.c_str(), uuid_parsed) != 0) {
    cerr << "Invalid UUID format: " << uuid_str << endl;
    return;
  }

  if (position < 1 || position > 4) {
    cerr << "Invalid player position: " << position << endl;
    return;
  }

  // Show player window if position is already occupied.
  if (!playerList.player[position - 1].empty()) {
    startPlayerThread(position - 1);
    return;
  }

  Json::Value req;
  req["path"] = "/api/v1/login";
  req["method"] = "POST";
  req["body"].append(uuid_str);
  req["body"].append(position);

  webSocket->send(req, [this, position](const Json::Value& response) {
    Json::Value user_data;
    Json::Reader().parse(response["body"].asString(), user_data);
    this->add(user_data["message"]["username"].asString(), position);
  });
}

void Player::add(const string username, int position)
{
  if (playerList.numPlayers < 4 && position >= 1 && position <= 4) {
    playerList.player[position - 1] = username;
    startPlayerThread(position - 1);
    ++playerList.numPlayers;
  }
}

/**
 * Run each player window/ countdown in a separate thread.
 *
 * @param index Player position index. Should be 0-3.
 */
void Player::startPlayerThread(int index)
{
  {
    lock_guard<mutex> lock(mtx);

    if (playerThread[index]) {
      cout << "Thread for player " << index << " is running." << endl;
      return;
    }

    playerThread[index] = true;
  }

  thread([this, index]() {
    showPlayerWindow(index);
    runTimer(TIMER_DEFAULT, index);
    hidePlayerWindow(index);
    {
      lock_guard<mutex> lock(mtx);
      playerThread[index] = false;
    }
  }).detach();
}

// vim: set ts=2 sw=2 expandtab:

