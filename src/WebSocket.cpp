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
#include "x11.h"
#include "Config.h"
#include "WebSocket.h"
#include "version.h"

using namespace std;

WebSocket::WebSocket(const string& uri) : baseUri(uri)
{
  ws.setUrl(uri);
  ws.setPingInterval(45);
  setHeaders();
  setupCallbacks();
  initDispatchers();
}

WebSocket::~WebSocket()
{
  stopPing();
  ws.stop();
}

void WebSocket::initDispatchers()
{
  // Logs the user out.
  cmdDispatchers["logout"] = [this](const Json::Value& payload) {
    if (payload.isMember("position")) playerHandler->logout(payload["position"].asInt());
  };

  // Displays a message on the screen.
  cmdDispatchers["message"] = [this](const Json::Value& payload) {
    if (payload.isMember("message")) {
      serverMessage = payload["message"].asString();
      startWindowThread(4);
    }
  };

  // Rotate authorization token.
  cmdDispatchers["token_rotate"] = [this](const Json::Value& payload) {
    if (payload.isMember("token") && payload.isMember("uuid")) {
      Json::Value config = payload;
      config.removeMember("cmd");
      rotateToken(config);
    }
  };

  // todo: Sign and verify payload signatures.
}

void WebSocket::setHeaders()
{
  ix::WebSocketHttpHeaders headers;
  headers["Content-Type"] = "application/json; charset=utf-8";

  if (!Config::machineId.empty() && !Config::token.empty()) {
    headers["Authorization"] = "Bearer " + Config::token;
    headers["X-Machine-Uuid"] = Config::machineId;
  }

  ws.setExtraHeaders(headers);
}

void WebSocket::rotateToken(const Json::Value& config)
{
  cout << "Updating token." << endl;
  Config::save(config);
  thread([this]() {
    this_thread::sleep_for(chrono::milliseconds(100));
    reconnect();
  })
  .detach();
}

void WebSocket::reconnect()
{
  ws.stop();
  Config::load();
  setHeaders();
  connect();
}

void WebSocket::setupCallbacks()
{
  ws.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
    switch (msg->type) {
    case ix::WebSocketMessageType::Error:
      lastError = msg->errorInfo.reason.empty() ? "Connection failed." : msg->errorInfo.reason;
      break;

    case ix::WebSocketMessageType::Open:
      connected.store(true);
      if (!Config::machineId.empty() && !Config::token.empty()) startPing();
      break;

    case ix::WebSocketMessageType::Close:
      connected.store(false);
      stopPing();
      if (msg->closeInfo.code == 4001) lastError = "Authentication failed.";
      break;

    case ix::WebSocketMessageType::Message: {
      Json::Value json;
      if (!Json::Reader().parse(msg->str, json)) break;

      // API response.
      if (json.isMember("request_id")) {
        if (validateApiResponse(json) == 0) processApiResponse(json);
        break;
      }

      // Server command.
      if (json.isMember("uuid") && json["uuid"].asString() == Config::machineId) {
        processCmd(json);
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

void WebSocket::processApiResponse(const Json::Value& json)
{
  lock_guard<mutex> lock(callbacksMtx);
  auto it = callbacks.find(json["request_id"].asString());
  if (it != callbacks.end()) {
    it->second(json);
    callbacks.erase(it);
  }
}

void WebSocket::processCmd(const Json::Value& payload)
{
  const string& cmd = payload["cmd"].asString();
  auto it = cmdDispatchers.find(cmd);
  if (it != cmdDispatchers.end()) {
    it->second(payload);
  }
  else {
    cerr << "Unknown command: " << cmd << endl;
  }
}

int WebSocket::validateApiResponse(const Json::Value& response)
{
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

void WebSocket::connect()
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
    throw runtime_error("Socket failed to connect: " + error);
  }
}

void WebSocket::send(const Json::Value& msg, Callback callback)
{
  if (!connected.load()) return;

  Json::Value sendmsg = msg;
  sendmsg["version"] = Version::FULL;

  if (callback) {
    uuid_t uuid;
    string reqid(37, '\0');

    uuid_generate_random(uuid);
    uuid_unparse_lower(uuid, &reqid[0]);
    reqid.resize(36);
    sendmsg["request_id"] = reqid;

    lock_guard<mutex> lock(callbacksMtx);
    callbacks[reqid] = callback;
  }

  Json::StreamWriterBuilder writerBuilder;
  writerBuilder["indentation"] = "";
  ws.send(Json::writeString(writerBuilder, sendmsg));
}

void WebSocket::startPing()
{
  if (pingThreadRunning.exchange(true)) return;

  pingThread = std::thread([this]() {
    while (pingThreadRunning.load() && connected.load()) {
      Json::Value req;
      req["path"] = "/api/v1/ping";
      req["method"] = "POST";

      this->send(req, [this](const Json::Value& response) {
        if (response["status"].asInt() != 200) {
          cerr << "Ping failed." << endl;
        }
      });

      {
        unique_lock<std::mutex> lock(pingMtx);
        pingCv.wait_for(lock, chrono::seconds(10), [this]() {
          return !pingThreadRunning.load() || !connected.load();
        });
      }
    }
  });
}

void WebSocket::stopPing()
{
  pingThreadRunning.store(false);
  pingCv.notify_one();
  if (pingThread.joinable() && pingThread.get_id() != this_thread::get_id()) {
    pingThread.join();
  }
}

// vim: set ts=2 sw=2 expandtab:

