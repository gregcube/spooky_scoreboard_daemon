
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

