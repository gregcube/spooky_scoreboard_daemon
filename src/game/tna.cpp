#include <iostream>

#include "yaml-cpp/yaml.h"
#include "tna.h"

const std::string TNA::name()
{
  return "Total Nuclear Annihilation";
}

const std::string TNA::highScoresFile()
{
  return "tna.yaml";
}

const Json::Value TNA::processHighScores()
{
  Json::Value root, classic, last;
  YAML::Node tnaNode = YAML::LoadFile("/tna/game/config/tna.yaml");

  YAML::Node classicScoresNode = tnaNode["ClassicHighScores"];
  YAML::Node lastScoresNode = tnaNode["LastScoreData"];

  for (std::size_t i = 0; i < classicScoresNode.size(); i++) {
    Json::Value score;

    score["initials"] = classicScoresNode[i]["inits"].as<std::string>();
    score["score"] = classicScoresNode[i]["score"].as<uint32_t>();

    classic.append(score);
    score.clear();
  }

  last.append(lastScoresNode["Player1LastScore"].as<uint32_t>());
  last.append(lastScoresNode["Player2LastScore"].as<uint32_t>());
  last.append(lastScoresNode["Player3LastScore"].as<uint32_t>());
  last.append(lastScoresNode["Player4LastScore"].as<uint32_t>());

  root.append(classic).append(last);
  return root;
}

const std::string TNA::getGamePath()
{
  return "/tna/game/config";
}

uint32_t TNA::getGamesPlayed()
{
  YAML::Node config = YAML::LoadFile("/tna/game/config/tna.yaml");
  return config["Audits"]["Games Played"].as<uint32_t>();
}
