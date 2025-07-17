#include <map>

#include "GameBase.h"

#include "game/hwn.h"
#include "game/um.h"
#include "game/tna.h"
#include "game/tcm.h"
#include "game/ed.h"

using namespace std;

std::map<std::string, GameFactoryFunction> gameFactories = {
  {"tna", []() { return make_unique<TNA>(); }},
  {"hwn", []() { return make_unique<HWN>(); }},
  {"tcm", []() { return make_unique<TCM>(); }},
  {"um",  []() { return make_unique<UM>();  }},
  {"ed",  []() { return make_unique<ED>();  }}
};

std::unique_ptr<GameBase> GameBase::create(const std::string& gameName)
{
  auto it = gameFactories.find(gameName);

  if (it != gameFactories.end()) {
    return (it->second)();
  }

  return nullptr;
}

