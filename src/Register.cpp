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

#include <json/json.h>
#include <sys/stat.h>

#include "Register.h"
#include "Config.h"

using namespace std;

future<void> Register::registerMachine(const string& regcode, const string& configPath)
{
  auto promise = make_shared<std::promise<void>>();
  auto future = promise->get_future();

  if (regcode.empty()) {
    promise->set_exception(make_exception_ptr(runtime_error("Missing registration code.")));
    return future;
  }

  if (regcode.size() != 4) {
    promise->set_exception(make_exception_ptr(runtime_error("Invalid registration code.")));
    return future;
  }

  size_t dir_end = configPath.find_last_of('/');
  if (dir_end == string::npos) {
    promise->set_exception(make_exception_ptr(runtime_error("Invalid path.")));
    return future;
  }

  string dir = configPath.substr(0, dir_end);

  // Halloween is old and std::filesystem isn't available.
  // Use POSIX C for better compatibility.
  struct stat st;
  if (stat(dir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
    promise->set_exception(make_exception_ptr(runtime_error("Directory does not exist.")));
    return future;
  }

  Json::Value msg;
  msg["path"] = "/api/v1/register";
  msg["method"] = "POST";
  msg["body"]["code"] = regcode;

  webSocket->send(msg, [promise, configPath](const Json::Value& response) {
    try {
      if (response["status"].asInt() != 200) {
        throw runtime_error("Registration failed.");
      }

      Json::Value config;
      Json::Reader().parse(response["body"].asString(), config);
      Config::save(config["message"], configPath);

      cout << "Machine registered." << endl;
      promise->set_value();
    }
    catch (const exception& e) {
      promise->set_exception(current_exception());
    }
  });

  return future;
}

// vim: set ts=2 sw=2 expandtab:

