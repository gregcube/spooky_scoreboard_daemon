#include <map>

#include "GameBase.h"
#include "game/hwn.h"
#include "game/tna.h"
#include "game/um.h"

std::map<std::string, GameFactoryFunction> gameFactories = {
  {"tna", []() { return std::make_unique<TNA>(); }},
  {"hwn", []() { return std::make_unique<HWN>(); }},
  {"um",  []() { return std::make_unique<UM>();  }}
};

std::unique_ptr<GameBase> GameBase::create(const std::string& gameName)
{
  auto it = gameFactories.find(gameName);

  if (it != gameFactories.end()) {
    return (it->second)();
  }

  return nullptr;
}

