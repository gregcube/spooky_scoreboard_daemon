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
#include <fstream>
#include <uuid/uuid.h>

#include "main.h"
#include "Config.h"

using namespace std;

string Config::machineId;
string Config::token;

void Config::load()
{
  string path = game->getGamePath() + "/" + configFile;

  ifstream file(path);
  if (!file.is_open()) {
    cerr << "Failed to open " << path << endl;
    exit(EXIT_FAILURE);
  }

  Json::Value root;
  Json::Reader reader;
  if (!reader.parse(file, root)) {
    cerr << "Failed to parse " << path << endl;
    exit(EXIT_FAILURE);
  }

  file.close();

  machineId = root["uuid"].asString();
  token = root["token"].asString();

  uuid_t uuid;
  if (machineId.empty() || uuid_parse(machineId.c_str(), uuid) != 0) {
    cerr << "Invalid machine UUID." << endl;
    exit(EXIT_FAILURE);
  }
}

void Config::save(const Json::Value& config)
{
  string path = game->getGamePath() + "/" + configFile;

  ofstream file(path);
  if (!file.is_open()) {
    cerr << "Failed to write " << path << endl;
    cout << "Config:\n" << config << endl;
    exit(EXIT_FAILURE);
  }

  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  file << Json::writeString(builder, config);
  file.close();

  cout << "Configuration saved." << endl;
}

// vim: set ts=2 sw=2 expandtab:

