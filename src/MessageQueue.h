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

#include <functional>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

#include <json/json.h>

class MessageQueue
{
public:
  using Callback = std::function<void(const Json::Value&)>;
  using SendHandler = std::function<void(const Json::Value&, Callback)>;

  explicit MessageQueue(SendHandler sendHandler);
  ~MessageQueue();

  void enqueue(Json::Value message, Callback callback = nullptr);
  void pause();
  void resume();

private:
  void process();

  SendHandler sendHandler;

  std::deque<std::pair<Json::Value, Callback>> queue;
  std::mutex mtx;
  std::condition_variable cv;
  std::thread workerThread;
 
  std::atomic<bool> running{false};
  std::atomic<bool> paused{false};
};

// vim: set ts=2 sw=2 expandtab:

