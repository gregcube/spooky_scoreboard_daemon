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

#pragma once

#include <ixwebsocket/IXWebSocket.h>
#include <json/json.h>

class WebSocketHandler
{
public:
  typedef std::function<void(const Json::Value&)> Callback;

  WebSocketHandler(const std::string& uri);
  ~WebSocketHandler();

  void connect();
  void send(const Json::Value& msg, Callback callback = nullptr);

  bool isConnected() const { return connected.load(); }
  std::string getLastError() const { return lastError; }

private:
  std::string baseUri;
  ix::WebSocket ws;

  std::atomic<bool> connected{false};
  std::atomic<bool> pingThreadRunning{false};

  std::thread pingThread;
  std::string lastError;
  std::mutex callbacksMtx, pingMtx;
  std::map<std::string, Callback> callbacks;
  std::condition_variable pingCv;

  void setupCallbacks();
  void startPingThread();
  void stopPingThread();
  int validate(const Json::Value& response);
};

// vim: set ts=2 sw=2 expandtab:

