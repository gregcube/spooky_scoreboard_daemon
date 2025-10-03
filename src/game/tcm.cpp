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

#include "game/tcm.h"

const Json::Value TCM::processHighScores()
{
  std::ifstream ifs((gamePath + "/" + highScoresFile).c_str());
  if (!ifs.is_open()) {
    throw std::runtime_error("Failed to process highscores");
  }

  std::string line;
  Json::Value scores;

  // Skip the first line.
  std::getline(ifs, line);

  // There are 6 scores.
  for (int i = 0; i < 6; i++) {
    Json::Value score;

    std::getline(ifs, line);
    score["initials"] = line;

    if (std::getline(ifs, line)) {
      score["score"] = line;
    }

    scores.append(score);
    score.clear();
  }

  // TODO: Implement mode scores.

  ifs.close();
  return scores;
}

const Json::Value TCM::processLastGameScores()
{
  std::ifstream ifs((gamePath + "/" + lastScoresFile).c_str());
  if (!ifs.is_open()) {
    throw std::runtime_error("Failed to process last game scores");
  }

  std::string line;
  Json::Value scores;

  // Skip first 14 lines. Last scores start at line 15
  for (int i = 0; i < 14 && std::getline(ifs, line); ++i) {}

  // Process next 4 lines for last scores.
  for (int i = 0; i < 4; ++i) {
    std::getline(ifs, line);
    scores.append(line);
  }

  ifs.close();
  return scores;
}

uint32_t TCM::getGamesPlayed()
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

int TCM::sendi3cmd()
{
  int sd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sd < 0) {
    std::cerr << "Failed to create socket" << std::endl;
    return -1;
  }

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;

  const char* i3path = getenv("I3_SOCKET_PATH");
  if (!i3path) {
    std::cerr << "I3_SOCKET_PATH environment variable not set" << std::endl;
    close(sd);
    return -1;
  }

  strncpy(addr.sun_path, i3path, sizeof(addr.sun_path) - 1);
  addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

  if (connect(sd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    std::cerr << "Failed to connect to I3 socket" << std::endl;
    close(sd);
    return -1;
  }

  std::vector<std::string> cmds = {
    "[title=\"TCM\"] border none",
    "[title=\"^Player [1-4]$\"] floating enable",
    "[title=\"^Player [1-4]$\"] sticky enable",
    "[title=\"^Player [1-4]$\"] border none",
    "[title=\"^Player [1-4]$\"] focus"
  };

  for (const auto& cmd : cmds) {
    std::string msg = "i3-ipc";
    uint32_t len = static_cast<uint32_t>(cmd.size());
    uint32_t type = 0;

    msg.append(reinterpret_cast<const char*>(&len), 4);
    msg.append(reinterpret_cast<const char*>(&type), 4);
    msg.append(cmd);

    ssize_t n = write(sd, msg.c_str(), msg.size());
    if (n < 0 || static_cast<size_t>(n) != msg.size()) {
      std::cerr << "Failed writing to I3 socket" << std::endl;
      close(sd);
      return -1;
    }
  }

  close(sd);
  return 0;
}

// vim: set ts=2 sw=2 expandtab:

