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
#include <json/json.h>

#include "RegistrationHandler.h"

using namespace std;

void RegistrationHandler::registerMachine(const string& regcode)
{
  if (regcode.empty()) return;

  Json::Value msg;
  msg["path"] = "/api/v1/register";
  msg["method"] = "POST";
  msg["body"] = regcode;

  webSocket->send(msg, [this](const Json::Value& response) {
    Json::Value config;
    Json::Reader().parse(response["body"].asString(), config);
    writeConfig(config["message"]);
  });
}

void RegistrationHandler::writeConfig(const Json::Value& config)
{
  ofstream file("/.ssbd.json");
  if (!file.is_open()) {
    cerr << "Failed to write /.ssbd.json." << endl;
    cout << "Copy/paste the below into /.ssbd.json" << endl;
    cout << config << endl;
    exit(EXIT_FAILURE);
  }

  Json::StreamWriterBuilder writer;
  file << Json::writeString(writer, config);
  file.close();

  cout << "File /.ssbd.json saved." << endl;
  cout << "Machine registered." << endl;
}

// vim: set ts=2 sw=2 expandtab:

