#include <iostream>

#include "yaml-cpp/yaml.h"
#include "tna.h"

const Json::Value TNA::processHighScores()
{
  Json::Value root, classic, last;
  YAML::Node tnaNode = YAML::LoadFile("/tna/game/config/tna.yaml");
  YAML::Node classicScoresNode = tnaNode["ClassicHighScores"];

  for (std::size_t i = 0; i < classicScoresNode.size(); i++) {
    Json::Value score;

    score["initials"] = classicScoresNode[i]["inits"].as<std::string>();
    score["score"] = classicScoresNode[i]["score"].as<uint32_t>();

    classic.append(score);
    score.clear();
  }

  // Process last scores.
  // Because the API expects highscore and last game scores submissions
  // together, we call processLastGameScores() here.
  // TODO: Update the API to have two endpoints for high and last game scores.
  last = processLastGameScores();

  root.append(classic);
  root.append(last);
  return root;
}

const Json::Value TNA::processLastGameScores()
{
  Json::Value scores;
  YAML::Node lastScoresNode = YAML::LoadFile("/tna/game/config/tna.yaml")["LastScoreData"];
  scores.append(lastScoresNode["Player1LastScore"].as<uint32_t>());
  scores.append(lastScoresNode["Player2LastScore"].as<uint32_t>());
  scores.append(lastScoresNode["Player3LastScore"].as<uint32_t>());
  scores.append(lastScoresNode["Player4LastScore"].as<uint32_t>());
  return scores;
}

uint32_t TNA::getGamesPlayed()
{
  YAML::Node config = YAML::LoadFile("/tna/game/config/tna.yaml");
  return config["Audits"]["Games Played"].as<uint32_t>();
}
