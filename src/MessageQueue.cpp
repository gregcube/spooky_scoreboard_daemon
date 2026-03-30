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

#include "MessageQueue.h"

MessageQueue::MessageQueue(SendHandler sh) : sendHandler(std::move(sh))
{
  running.store(true);
  workerThread = std::thread(&MessageQueue::process, this);
}

MessageQueue::~MessageQueue()
{
  {
    std::lock_guard<std::mutex> lock(mtx);
    running.store(false);
  }

  cv.notify_all();
  if (workerThread.joinable()) workerThread.join();
}

void MessageQueue::process()
{
  while (running.load()) {
    std::pair<Json::Value, Callback> item;
    {
      std::unique_lock<std::mutex> lock(mtx);
      cv.wait(lock, [this] { return !queue.empty() || !running.load(); });
      if (!running.load()) break;

      cv.wait(lock, [this] { return !paused.load() || !running.load(); });
      if (!running.load()) break;
      if (queue.empty()) continue;

      item = std::move(queue.front());
      queue.pop_front();
    }

    if (sendHandler) {
      std::cout << "Sending message from queue." << std::endl;
      sendHandler(std::move(item.first), std::move(item.second));
    }
  }
}

void MessageQueue::enqueue(Json::Value message, Callback callback)
{
#ifdef DEBUG
  std::cout << "Adding " << message << " to queue." << std::endl;
#endif
  {
    std::lock_guard<std::mutex> lock(mtx);
    queue.emplace_back(std::move(message), std::move(callback));
  }
  cv.notify_one();
}

void MessageQueue::pause()
{
  std::cout << "Pausing queue." << std::endl;
  paused.store(true);
}

void MessageQueue::resume()
{
  std::cout << "Resuming queue." << std::endl;
  paused.store(false);
  cv.notify_all();
}

// vim: set ts=2 sw=2 expandtab:

