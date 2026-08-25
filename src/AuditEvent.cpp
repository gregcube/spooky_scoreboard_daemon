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

#include <ctime>
#include <iostream>
#include <utility>

#include "main.h"
#include "AuditEvent.h"

using namespace std;

AuditEvent::AuditEvent(string eventType, Json::Value eventDetails)
  : type(std::move(eventType)),
    timestamp(static_cast<Json::Value::Int64>(time(nullptr))),
    details(std::move(eventDetails)) {}

Json::Value AuditEvent::toJson() const
{
  Json::Value json;
  json["type"] = type;
  json["timestamp"] = timestamp;
  if (!details.empty()) json["details"] = details;
  return json;
}

void AuditEvent::send() const
{
  if (!webSocket) return;

  Json::Value req;
  req["path"] = "/api/v1/log";
  req["method"] = "POST";
  req["query"] = "type=audit";
  req["body"] = toJson();

  webSocket->enqueueMessage(req, [](const Json::Value& response) {
    if (response["status"].asInt() != 200) {
      cerr << "Failed to upload audit event." << endl;
    }
  });
}

void AuditEvent::record(string eventType, Json::Value eventDetails)
{
  AuditEvent(std::move(eventType), std::move(eventDetails)).send();
}

// vim: set ts=2 sw=2 expandtab:
