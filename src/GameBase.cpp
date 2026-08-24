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

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>

#include <yaml-cpp/yaml.h>

#include "main.h"
#include "GameBase.h"

#include "game/EvilDead.h"
#include "game/Halloween.h"
#include "game/TexasChainsawMassacre.h"
#include "game/TotalNuclearAnnihilation.h"
#include "game/Ultraman.h"
#include "game/AliceCooperNightmareCastle.h"

using namespace std;

Json::Value GameBase::yamlToJson(const YAML::Node& node)
{
  switch (node.Type()) {
  case YAML::NodeType::Sequence: {
    Json::Value arr(Json::arrayValue);
    for (const auto& child : node)
      arr.append(yamlToJson(child));
    return arr;
  }
  case YAML::NodeType::Map: {
    Json::Value obj(Json::objectValue);
    for (const auto& kv : node)
      obj[kv.first.as<string>()] = yamlToJson(kv.second);
    return obj;
  }
  case YAML::NodeType::Scalar: {
    const string& s = node.Scalar();

    if (s == "true" || s == "True" || s == "TRUE")
      return Json::Value(true);
    if (s == "false" || s == "False" || s == "FALSE")
      return Json::Value(false);

    if (!s.empty()) {
      char* end = nullptr;
      errno = 0;
      const long long i = strtoll(s.c_str(), &end, 10);
      if (end != s.c_str() && *end == '\0' && errno != ERANGE)
        return Json::Value(static_cast<Json::Int64>(i));

      errno = 0;
      const double d = strtod(s.c_str(), &end);
      if (end != s.c_str() && *end == '\0' && errno != ERANGE)
        return Json::Value(d);
    }

    return Json::Value(s);
  }
  case YAML::NodeType::Null:
  case YAML::NodeType::Undefined:
  default:
    return Json::Value::null;
  }
}

std::map<std::string, GameFactoryFunction> gameFactories = {
  {"tna", []() { return make_unique<TotalNuclearAnnihilation>(); }},
  {"hwn", []() { return make_unique<Halloween>(); }},
  {"tcm", []() { return make_unique<TexasChainsawMassacre>(); }},
  {"um",  []() { return make_unique<Ultraman>(); }},
  {"ed",  []() { return make_unique<EvilDead>(); }},
  {"acnc",[]() { return make_unique<AliceCooperNightmareCastle>(); }}
};

std::unique_ptr<GameBase> GameBase::create(const std::string& gameName)
{
  auto it = gameFactories.find(gameName);

  if (it != gameFactories.end()) {
    return (it->second)();
  }

  return nullptr;
}

void GameBase::uploadScores(const Json::Value& scores, ScoreType type)
{
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

    webSocket->enqueueMessage(req, [this](const Json::Value& response) {
      if (response["status"].asInt() != 200) {
        cerr << "Failed to upload scores." << endl;
      }
    });
  }
  catch (const runtime_error& e) {
    cerr << "Exception: " << e.what() << endl;
  }

}

const Json::Value GameBase::processAudits()
{
  const string path = scoresPath + "/" + auditsFile;

  ifstream ifs(path);
  if (!ifs.is_open()) {
    throw runtime_error("Failed to open game audits file");
  }

  Json::Value root;
  Json::CharReaderBuilder builder;
  JSONCPP_STRING errs;
  if (!parseFromStream(builder, ifs, &root, &errs)) {
    throw runtime_error("Failed to read audits file");
  }

  return root;
}

void GameBase::uploadAudits()
{
  try {
    cout << "Uploading audits..." << endl;

    Json::Value req;
    req["path"] = "/api/v1/audits";
    req["method"] = "POST";
    req["body"] = processAudits();

    webSocket->enqueueMessage(req, [](const Json::Value& response) {
      if (response["status"].asInt() != 200) {
        cerr << "Failed to upload audits." << endl;
      }
    });
  }
  catch (const runtime_error& e) {
    cerr << "Exception: " << e.what() << endl;
  }
}

// vim: set ts=2 sw=2 expandtab:

