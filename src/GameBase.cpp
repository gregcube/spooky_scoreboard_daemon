#include <map>

#include "GameBase.h"

#include "game/hwn.h"
#include "game/tna.h"

std::map<std::string, GameFactoryFunction> gameFactories = {
  {"tna", []() { return std::make_unique<TNA>(); }},
  {"hwn", []() { return std::make_unique<HWN>(); }}
};

std::unique_ptr<GameBase> GameBase::create(const std::string& gameName)
{
  auto it = gameFactories.find(gameName);

  if (it != gameFactories.end()) {
    return (it->second)();
  }

  return nullptr;
}

