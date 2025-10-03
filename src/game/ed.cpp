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
#include <fstream>
#include <stdexcept>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <json/json.h>

#include "game/ed.h"

const Json::Value ED::processHighScores()
{
  std::ifstream ifs("/game/audits/highscores.json");
  if (!ifs.is_open()) {
    throw std::runtime_error("Failed to open highscores file.");
  }

  Json::Value highscores;
  Json::Reader reader;
  if (reader.parse(ifs, highscores) == false) {
    ifs.close();
    throw std::runtime_error("Failed to parse highscores file.");
  }

  ifs.close();

  /*
  [
    {
      "theScore": 275416160,
      "playerName": "GRG    ",
      "modeName": null,
      "playerIndex": 1,
      "scoreBeaten": false,
      "scorePlace": -1
    },
    ...
  ]
  */

  Json::Value scores;
  for (int i = 0; i < 6; i++) {
    Json::Value score;
    score["initials"] = highscores[i]["playerName"];
    score["score"] = highscores[i]["theScore"];
    scores.append(score);
    score.clear();
  }

  return scores;
}

const Json::Value ED::processLastGameScores()
{
  std::ifstream ifs("/game/audits/lastscores.json");
  if (!ifs.is_open()) {
    throw std::runtime_error("Failed to open last game scores file.");
  }

  Json::Value scores;
  Json::Reader reader;
  if (reader.parse(ifs, scores) == false) {
    ifs.close();
    throw std::runtime_error("Failed to parse last game scores file.");
  }

  ifs.close();
  return scores;
}

uint32_t ED::getGamesPlayed()
{
  std::ifstream ifs("/game/audits/_game_audits.json");

  if (!ifs.is_open()) {
    throw std::runtime_error("Failed to open game audits file");
  }

  Json::Value root;
  Json::CharReaderBuilder builder;
  JSONCPP_STRING errs;

  if (!parseFromStream(builder, ifs, &root, &errs)) {
    ifs.close();
    throw std::runtime_error("Failed to read audits file");
  }

  ifs.close();
  return root["games_played"]["value"].asUInt();
}

int ED::sendswaycmd()
{
  int sd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sd < 0) {
    std::cerr << "Failed to create socket" << std::endl;
    return -1;
  }

  struct sockaddr_un addr = {};
  addr.sun_family = AF_UNIX;

  const char* swaypath = getenv("SWAYSOCK");
  if (!swaypath) {
    std::cerr << "SWAYSOCK not set" << std::endl;
    close(sd);
    return -1;
  }

  strncpy(addr.sun_path, swaypath, sizeof(addr.sun_path) - 1);

  if (connect(sd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    std::cerr << "Failed to connect to sway socket" << std::endl;
    close(sd);
    return -1;
  }

  std::vector<std::string> cmds = {
    "[title=\"Evil Dead\"] border none",
    "[title=\"^Player [1-4]$\"] floating enable",
    "[title=\"^Player [1-4]$\"] sticky enable",
    "[title=\"^Player [1-4]$\"] border none",
    "[title=\"^Player [1-4]$\"] focus"
  };

  for (const auto& cmd : cmds) {
    std::string msg = "i3-ipc";
    uint32_t len = static_cast<uint32_t>(cmd.size());
    uint32_t type = 0;

    // Ensure little-endian for length and type.
    uint32_t len_le = htole32(len);
    uint32_t type_le = htole32(type);

    msg.append(reinterpret_cast<const char*>(&len_le), 4);
    msg.append(reinterpret_cast<const char*>(&type_le), 4);
    msg.append(cmd);

    if (write(sd, msg.c_str(), msg.size()) != static_cast<ssize_t>(msg.size())) {
      std::cerr << "Failed writing to sway socket" << std::endl;
      close(sd);
      return -1;
    }

    // Read response to ensure command was processed.
    char buffer[1024];
    ssize_t n = read(sd, buffer, sizeof(buffer));
    if (n <= 0) {
      std::cerr << "Failed reading response" << std::endl;
      close(sd);
      return -1;
    }
  }

  close(sd);
  return 0;
}

// vim: set ts=2 sw=2 expandtab:

