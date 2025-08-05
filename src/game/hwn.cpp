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

#include <json/json.h>

#include "game/hwn.h"

const Json::Value HWN::processHighScores()
{
  std::ifstream ifs("/game/highscores.config");
  if (!ifs.is_open()) {
    throw std::runtime_error("Failed to process highscores");
  }

  std::string line;
  Json::Value root;

  // Process classic scores.
  Json::Value classicScores;

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

    classicScores.append(score);
    score.clear();
  }

  // Process last scores.
  Json::Value lastScores;

  // Skip this line.
  // It should be "[LAST SCORES]"
  std::getline(ifs, line);

  // There are 4 scores.
  for (int i = 0; i < 4; i++) {
    std::getline(ifs, line);
    lastScores.append(line);
  }

  // TODO: Implement mode scores.

  root.append(classicScores);
  root.append(lastScores);
  ifs.close();

  return root;
}

const Json::Value HWN::processLastGameScores()
{
  std::ifstream ifs("/game/highscores.config");
  if (!ifs.is_open()) {
    throw std::runtime_error("Failed to process last game scores");
  }

  std::string line;
  Json::Value scores;

  // Skip first 8 lines.
  for (int i = 0; i < 8 && std::getline(ifs, line); ++i) {}

  // 4 next lines are last player scores.
  for (int i = 0; i < 4; i++) {
    std::getline(ifs, line);
    scores.append(line);
  }

  ifs.close();
  return scores;
}

uint32_t HWN::getGamesPlayed()
{
  std::ifstream ifs("/game/_game_audits.json");

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

// vim: set ts=2 sw=2 expandtab:

