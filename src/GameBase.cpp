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

#include <iostream>
#include <map>

#include "main.h"
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

void GameBase::setUrl(WebSocket* ws)
{
  if (!ws) return;

  Json::Value req;
  req["path"] = "/api/v1/url";
  req["method"] = "GET";

  ws->send(req, [this](const Json::Value& response) {
    if (response["status"].asInt() == 200) {
      Json::Value msg;
      Json::Reader().parse(response["body"].asString(), msg);
      size_t len = msg["message"].asString().size();
      gameUrl = msg["message"].asString().substr(8, len - 8);
    }
  });
}

void GameBase::uploadScores(const Json::Value& scores, ScoreType type, WebSocket* ws)
{
  if (!ws) return;

  cout << "Uploading scores..." << endl;

  try {
    Json::Value req;

    string query = "type=";
    switch (type) {
    case ScoreType::High: query += "classic"; break;
    case ScoreType::Last: query += "last"; break;
    case ScoreType::Mode: query += "mode"; break;
    }

    req["path"] = "/api/v1/score";
    req["method"] = "POST";
    req["query"] = query;
    req["body"] = scores;

    webSocket->send(req, [this](const Json::Value& response) {
      if (response["status"].asInt() != 200) {
        cerr << "Failed to upload scores." << endl;
      }
    });
  }
  catch (const runtime_error& e) {
    cerr << "Exception: " << e.what() << endl;
  }

}

// vim: set ts=2 sw=2 expandtab:

