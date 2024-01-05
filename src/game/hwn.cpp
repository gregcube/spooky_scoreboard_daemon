#include <iostream>
#include <fstream>
#include <stdexcept>

#include <json/json.h>

#include "hwn.h"

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

  root.append(classicScores).append(lastScores);
  ifs.close();

  return root;
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

