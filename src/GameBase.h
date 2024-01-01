#ifndef _GAME_BASE_H
#define _GAME_BASE_H

#include <string>
#include <memory>

#include <json/json.h>

class GameBase
{
public:
  virtual ~GameBase() = default;
  virtual const std::string name() = 0;
  virtual const std::string getGamePath() = 0;
  virtual const std::string highScoresFile() = 0;
  virtual const Json::Value processHighScores() = 0;
  virtual uint32_t getGamesPlayed() = 0;
  static std::unique_ptr<GameBase> create(const std::string& gameName);
};

#endif
