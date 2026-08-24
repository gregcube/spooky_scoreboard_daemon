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

#include <array>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>

#include <json/json.h>

#include "main.h"
#include "Journal.h"

using namespace std;

namespace {

constexpr int kMaxLines = 500;
constexpr size_t kMaxBytes = 256 * 1024;

string trimToMaxBytes(string log)
{
  if (log.size() <= kMaxBytes) return log;

  size_t cut = log.size() - kMaxBytes;
  const size_t nl = log.find('\n', cut);
  if (nl != string::npos) cut = nl + 1;

  log.erase(0, cut);
  return log;
}

string readJournal()
{
  const string cmd =
    "/usr/bin/journalctl --unit=ssbd.service --lines=" + to_string(kMaxLines) +
    " --no-pager --output=short-iso --quiet";

  FILE* fp = popen(cmd.c_str(), "r");
  if (!fp) throw runtime_error("Failed to open ssbd journal");

  ostringstream ss;
  array<char, 4096> buf{};
  while (fgets(buf.data(), static_cast<int>(buf.size()), fp) != nullptr)
    ss << buf.data();

  const int rc = pclose(fp);
  if (rc == -1 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
    throw runtime_error("Failed to read ssbd journal");

  return trimToMaxBytes(ss.str());
}

} // namespace

void uploadJournal()
{
  try {
    cout << "Uploading ssbd journal..." << endl;

    Json::Value req;
    req["path"] = "/api/v1/log";
    req["method"] = "POST";
    req["query"] = "type=ssbd";
    req["body"] = readJournal();

    webSocket->enqueueMessage(req, [](const Json::Value& response) {
      if (response["status"].asInt() != 200) {
        cerr << "Failed to upload ssbd journal." << endl;
      }
    });
  }
  catch (const runtime_error& e) {
    cerr << "Exception: " << e.what() << endl;
  }
}

// vim: set ts=2 sw=2 expandtab:
