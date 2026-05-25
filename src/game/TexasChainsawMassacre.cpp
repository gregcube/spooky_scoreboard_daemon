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

#include "game/TexasChainsawMassacre.h"

const Json::Value TexasChainsawMassacre::processHighScores()
{
  std::ifstream ifs((scoresPath + "/" + highScoresFile).c_str());
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

const Json::Value TexasChainsawMassacre::processLastGameScores()
{
  std::ifstream ifs((scoresPath + "/" + lastScoresFile).c_str());
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

uint32_t TexasChainsawMassacre::getGamesPlayed()
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

// vim: set ts=2 sw=2 expandtab:

