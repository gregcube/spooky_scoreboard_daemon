#ifndef _GAME_BASE_H
#define _GAME_BASE_H

#include <string>
#include <memory>
#include <functional>

#include <json/json.h>

class GameBase
{
public:
  virtual ~GameBase() = default;
  virtual const Json::Value processHighScores() = 0;
  virtual uint32_t getGamesPlayed() = 0;

  static std::unique_ptr<GameBase> create(const std::string& gameName);

  virtual const std::string& getGameName() = 0;
  virtual const std::string& getGamePath() = 0;
  virtual const std::string& getTmpPath() = 0;
  virtual const std::string& getAuditsFile() = 0;
};

using GameFactoryFunction = std::function<std::unique_ptr<GameBase>()>;

extern std::map<std::string, GameFactoryFunction> gameFactories;

#endif
