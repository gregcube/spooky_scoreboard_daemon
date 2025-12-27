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

#include "Register.h"
#include "Config.h"

std::future<void> Register::registerMachine(const std::string& regcode, const std::string& configPath)
{
  auto promise = std::make_shared<std::promise<void>>();
  auto future = promise->get_future();

  if (regcode.empty()) {
    promise->set_exception(std::make_exception_ptr(std::runtime_error("Missing registration code.")));
    return future;
  }

  Json::Value msg;
  msg["path"] = "/api/v1/register";
  msg["method"] = "POST";
  msg["body"]["code"] = regcode;

  webSocket->send(msg, [promise, configPath](const Json::Value& response) {
    try {
      if (response["status"].asInt() != 200) {
        throw std::runtime_error("Registration failed.");
      }

      Json::Value config;
      Json::Reader().parse(response["body"].asString(), config);
      Config::save(config["message"], configPath);

      std::cout << "Machine registered." << std::endl;
      promise->set_value();
    }
    catch (const std::exception& e) {
      promise->set_exception(std::current_exception());
    }
  });

  return future;
}

// vim: set ts=2 sw=2 expandtab:

