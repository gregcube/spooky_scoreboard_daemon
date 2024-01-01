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
  /*
  YAML::Node highscores = YAML::LoadFile("sample/tna.yaml");
  std::cout << "Score" << highscores["ClassicHighScores"][0]["score"].as<std::string>() << "\n";
  */
  // TODO: Convert yaml to json in expected web server format.
  Json::Value root;
  return root;
}

const std::string TNA::getGamePath()
{
  return "/tna/game/config";
}

uint32_t TNA::getGamesPlayed()
{
  YAML::Node config = YAML::LoadFile("sample/tna.yaml");
  return config["Audits"]["Games Played"].as<uint32_t>();
}
