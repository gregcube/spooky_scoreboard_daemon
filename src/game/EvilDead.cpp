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

#include "game/EvilDead.h"

const Json::Value EvilDead::processHighScores()
{
  std::ifstream ifs((scoresPath + "/" + highScoresFile).c_str());
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

const Json::Value EvilDead::processLastGameScores()
{
  std::ifstream ifs((scoresPath + "/" + lastScoresFile).c_str());
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

uint32_t EvilDead::getGamesPlayed()
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

