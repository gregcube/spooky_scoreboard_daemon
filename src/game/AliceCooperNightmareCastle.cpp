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

#include <yaml-cpp/yaml.h>

#include "game/AliceCooperNightmareCastle.h"

const Json::Value AliceCooperNightmareCastle::processHighScores()
{
  Json::Value scores;
  YAML::Node classicHighScores;
  std::string path(scoresPath + "/" + highScoresFile);

  try {
    classicHighScores = YAML::LoadFile(path)["ClassicHighScores"];
  }
  catch (const YAML::Exception& e) {
    std::cerr << "Failed to load YAML: " << path << "\n" << e.what() << std::endl;
    throw std::runtime_error("Failed to load high scores from YAML file.");
  }

  for (std::size_t i = 0; i < classicHighScores.size(); i++) {
    Json::Value score;

    score["initials"] = classicHighScores[i]["inits"].as<std::string>();
    score["score"] = classicHighScores[i]["score"].as<uint32_t>();

    scores.append(score);
    score.clear();
  }

  return scores;
}

const Json::Value AliceCooperNightmareCastle::processLastGameScores()
{
  Json::Value scores;
  YAML::Node lastScoreData;
  std::string path(scoresPath + "/" + lastScoresFile);

  try {
    lastScoreData = YAML::LoadFile(path)["LastScoreData"];
  }
  catch (const YAML::Exception& e) {
    std::cerr << "Failed to load YAML: " << path << "\n" << e.what() << std::endl;
    throw std::runtime_error("Failed to load last scores from YAML file.");
  }

  scores.append(lastScoreData["Player1LastScore"].as<uint32_t>());
  scores.append(lastScoreData["Player2LastScore"].as<uint32_t>());
  scores.append(lastScoreData["Player3LastScore"].as<uint32_t>());
  scores.append(lastScoreData["Player4LastScore"].as<uint32_t>());

  return scores;
}

uint32_t AliceCooperNightmareCastle::getGamesPlayed()
{
  YAML::Node audits;
  std::string path("/game/code/config/" + auditsFile);

  try {
    audits = YAML::LoadFile(path)["Audits"];
  }
  catch (const YAML::Exception& e) {
    std::cerr << "Failed to load YAML: " << path << "\n" << e.what() << std::endl;
    throw std::runtime_error("Failed to load audits from YAML file.");
  }

  return audits["Games Played"].as<uint32_t>();
}

// vim: set ts=2 sw=2 expandtab:

