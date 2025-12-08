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
#include <uuid/uuid.h>

#include "main.h"
#include "WebSocketHandler.h"

using namespace std;

WebSocketHandler::WebSocketHandler(const string& uri) : baseUri(uri)
{
  ws.setUrl(uri);
  ws.setPingInterval(10);

  ix::WebSocketHttpHeaders headers;
  headers["Content-Type"] = "application/json; charset=utf-8";

  if (!machineId.empty() && !token.empty()) {
    headers["Authorization"] = "Bearer " + token;
    headers["X-Machine-Uuid"] = machineId;
  }

  ws.setExtraHeaders(headers);

  setupCallbacks();
}

WebSocketHandler::~WebSocketHandler()
{
  stopPingThread();
  ws.stop();
}

void WebSocketHandler::setupCallbacks()
{
  ws.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
    switch (msg->type) {
    case ix::WebSocketMessageType::Error:
      lastError = msg->errorInfo.reason.empty() ? "Connection failed" : msg->errorInfo.reason;
      break;

    case ix::WebSocketMessageType::Open:
      connected.store(true);
      startPingThread();
      break;

    case ix::WebSocketMessageType::Close:
      connected.store(false);
      stopPingThread();
      break;

    case ix::WebSocketMessageType::Message: {
      Json::Value response;
      Json::Reader reader;

      if (reader.parse(msg->str, response) && validate(response) == 0) {
        string request_id = response["request_id"].asString();
        lock_guard<mutex> lock(mtx);

        auto it = callbacks.find(request_id);
        it->second(response);
        callbacks.erase(it);
      }
      break;
    }

    case ix::WebSocketMessageType::Ping:
      break;

    case ix::WebSocketMessageType::Pong:
      break;

    case ix::WebSocketMessageType::Fragment:
      break;
    }
  });
}

int WebSocketHandler::validate(const Json::Value& response)
{
  int rc = response["status"].asInt();
  if (rc != 200) {
    cerr << "Error: " << rc << endl << response["body"] << endl;
    return 1;
  }

  string request_id = response["request_id"].asString();
  uuid_t uuid;

  if (request_id.empty() ||
      uuid_parse(request_id.c_str(), uuid) != 0 ||
      callbacks.find(request_id) == callbacks.end()) {

    cerr << "Error: Missing or invalid request id." << endl;
    return 1;
  }

  return 0;
}

void WebSocketHandler::connect()
{
  connected.store(false);
  lastError.clear();
  ws.start();

  auto timeout = chrono::steady_clock::now() + chrono::seconds(10);
  while (chrono::steady_clock::now() < timeout) {
    if (connected.load()) return;
    if (!lastError.empty()) break;
    this_thread::sleep_for(chrono::milliseconds(50));
  }

  if (!connected.load()) {
    string error = lastError.empty() ? "timeout or unknown error." : lastError;
    throw runtime_error("Websocket failed to connect: " + error);
  }
}

void WebSocketHandler::send(const Json::Value& msg, Callback callback)
{
  if (!connected.load()) return;

  Json::Value sendmsg = msg;

  if (callback) {
    uuid_t uuid;
    string reqid(37, '\0');

    uuid_generate_random(uuid);
    uuid_unparse_lower(uuid, &reqid[0]);
    reqid.resize(36);
    sendmsg["request_id"] = reqid;

    lock_guard<mutex> lock(mtx);
    callbacks[reqid] = callback;
  }

  Json::StreamWriterBuilder writerBuilder;
  writerBuilder["indentation"] = "";
  ws.send(Json::writeString(writerBuilder, sendmsg));
}

void WebSocketHandler::startPingThread()
{
  if (pingThreadRunning.exchange(true)) return;

  pingThread = std::thread([this]() {
    while (pingThreadRunning.load() && connected.load()) {
      Json::Value req;
      req["path"] = "/api/v1/ping";
      req["method"] = "POST";

      this->send(req, [this](const Json::Value& response) {
        // todo: Check response for "OK"
      });

      std::this_thread::sleep_for(std::chrono::seconds(10));
    }
  });
}

void WebSocketHandler::stopPingThread()
{
  pingThreadRunning.store(false);
  if (pingThread.joinable()) pingThread.join();
}

// vim: set ts=2 sw=2 expandtab:

