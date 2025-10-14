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

#include <map>

#include "GameBase.h"

#include "game/EvilDead.h"
#include "game/Halloween.h"
#include "game/TexasChainsawMassacre.h"
#include "game/TotalNuclearAnnihilation.h"
#include "game/Ultraman.h"

using namespace std;

std::map<std::string, GameFactoryFunction> gameFactories = {
  {"tna", []() { return make_unique<TotalNuclearAnnihilation>(); }},
  {"hwn", []() { return make_unique<Halloween>(); }},
  {"tcm", []() { return make_unique<TexasChainsawMassacre>(); }},
  {"um",  []() { return make_unique<Ultraman>(); }},
  {"ed",  []() { return make_unique<EvilDead>(); }}
};

std::unique_ptr<GameBase> GameBase::create(const std::string& gameName)
{
  auto it = gameFactories.find(gameName);

  if (it != gameFactories.end()) {
    return (it->second)();
  }

  return nullptr;
}

