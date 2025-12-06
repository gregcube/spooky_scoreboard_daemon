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
#include "WebSocketHandler.h"

WebSocketHandler::WebSocketHandler(const std::string& uri) : baseUri(uri)
{
  ws.setUrl(uri);
  ws.setPingInterval(10);
  setupCallbacks();
}

WebSocketHandler::~WebSocketHandler()
{
  ws.stop();
}

void WebSocketHandler::setupCallbacks()
{
  ws.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
    switch (msg->type) {
    case ix::WebSocketMessageType::Error:
      lastError = msg->errorInfo.reason.empty() ? "Connection failed" : msg->errorInfo.reason;
      connected.store(false);
      break;

    case ix::WebSocketMessageType::Open:
      connected.store(true);
      break;

    case ix::WebSocketMessageType::Close:
      connected.store(false);
      break;

    case ix::WebSocketMessageType::Message:
      break;

    case ix::WebSocketMessageType::Ping:
      std::cout << "Ping: " + msg->str << std::endl;
      break;

    case ix::WebSocketMessageType::Pong:
      std::cout << "Pong: " + msg->str << std::endl;
      break;

    case ix::WebSocketMessageType::Fragment:
      break;
    }
  });
}

void WebSocketHandler::connect()
{
  connected.store(false);
  lastError.clear();
  ws.start();

  auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (std::chrono::steady_clock::now() < timeout) {
    if (connected.load()) return;
    if (!lastError.empty()) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (!connected.load()) {
    std::string error = lastError.empty() ? "timeout or unknown error." : lastError;
    throw std::runtime_error("Websocket failed to connect: " + error);
  }
}

void WebSocketHandler::send(const Json::Value& msg)
{
  if (!connected.load()) return;

  // todo: Add http headers using WebSocketHttpHeaders.

  Json::StreamWriterBuilder writerBuilder;
  writerBuilder["indentation"] = "";

  ws.send(Json::writeString(writerBuilder, msg));
}

// vim: set ts=2 sw=2 expandtab:

