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
#include "Signature.h"
#include "version.h"

using namespace std;

WebSocket::WebSocket(const string& uri) : baseUri(uri)
{
  ws.setUrl(uri);
  ws.setPingInterval(45);

  setHeaders();
  setupCallbacks();
  initDispatchers();

  auto sendHandler = [this](const Json::Value& msg, Callback cb) { send(std::move(msg), std::move(cb)); };
  messageQueue = std::make_unique<MessageQueue>(std::move(sendHandler));
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
      Json::Value config;
      config["token"] = payload["token"];
      config["uuid"] = payload["uuid"];
      tokenRotate(config);
    }
  };
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

std::future<bool> WebSocket::isTokenExpired()
{
  auto promise = make_shared<std::promise<bool>>();
  auto future = promise->get_future();

  Json::Value req;
  req["path"] = "/api/v1/token";
  req["method"] = "POST";

  send(req, [promise](const Json::Value& response) {
    try {
      if (response["status"].asInt() == 200) {
        Json::Value body;
        Json::Reader().parse(response["body"].asString(), body);
        promise->set_value(body["message"].asInt() <= 0);
      }
      else {
        throw runtime_error("Failed token check.");
      }
    }
    catch (...) {
      promise->set_exception(current_exception());
    }
  });

  return future;
}

std::future<void> WebSocket::waitForTokenRotate()
{
  lock_guard<mutex> lock(tokenRotateMtx);
  if (!tokenRotatePromise) tokenRotatePromise = make_shared<promise<void>>();
  return tokenRotatePromise->get_future();
}

void WebSocket::tokenRotate(const Json::Value& config)
{
  messageQueue->pause();

  cout << "Rotating token..." << endl;
  Config::save(config);

  thread([this]() {
    reconnect();

    {
      lock_guard<mutex> lock(tokenRotateMtx);
      if (tokenRotatePromise) {
        tokenRotatePromise->set_value();
        tokenRotatePromise.reset();
      }
    }

    messageQueue->resume();
  })
  .detach();
}

void WebSocket::reconnect()
{
  stopPing();
  ws.stop();
  Config::load();
  setHeaders();
  connect();
  if (!Config::machineId.empty() && !Config::token.empty()) {
    startPing();
  }
}

void WebSocket::setupCallbacks()
{
  ws.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
    switch (msg->type) {
    case ix::WebSocketMessageType::Error:
      lastError = msg->errorInfo.reason.empty() ? "Error." : msg->errorInfo.reason;
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

      // Verify signature.
      if (!json.isMember("register") && !Signature::verify(json)) {
        cerr << "Message: invalid signature." << endl;
        break;
      }

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
    string error = lastError.empty() ? "Timeout or unknown error." : lastError;
    throw runtime_error("Socket failed to connect: " + error);
  }
}

void WebSocket::enqueueMessage(const Json::Value& msg, Callback callback)
{
  messageQueue->enqueue(msg, callback);
}

void WebSocket::send(Json::Value msg, Callback callback)
{
  if (!connected.load()) return;

  msg["version"] = Version::FULL;
  msg["uuid"] = Config::machineId;
  msg["timestamp"] = static_cast<Json::Value::Int64>(time(nullptr));

  uuid_t uuid;
  char nonce[37];
  uuid_generate_random(uuid);
  uuid_unparse_lower(uuid, nonce);
  msg["nonce"] = nonce;

  if (callback) {
    memset(uuid, 0, sizeof(uuid_t));
    char reqid[37];

    uuid_generate_random(uuid);
    uuid_unparse_lower(uuid, reqid);
    msg["request_id"] = reqid;

    lock_guard<mutex> lock(callbacksMtx);
    callbacks[reqid] = callback;
  }

  if (!Config::token.empty()) {
    msg["signature"] = Signature::sign(msg);
  }

  Json::StreamWriterBuilder writerBuilder;
  writerBuilder["indentation"] = "";
  ws.send(Json::writeString(writerBuilder, msg));
}

void WebSocket::startPing()
{
  stopPing();
  if (pingThreadRunning.exchange(true)) return;

  pingThread = thread([this]() {
    cout << "Start ping thread..." << endl;

    Json::Value req;
    req["path"] = "/api/v1/ping";
    req["method"] = "POST";

    while (pingThreadRunning.load() && connected.load()) {
      this->send(req, [this](const Json::Value& response) {
        if (response["status"].asInt() != 200) {
          cerr << "Ping failed: " << response["error"].asString() << endl;
        }
      });

      {
        unique_lock<mutex> lock(pingMtx);
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
    cout << "Stop ping thread..." << endl;
    pingThread.join();
  }
}

// vim: set ts=2 sw=2 expandtab:

