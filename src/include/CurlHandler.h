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

#ifndef _CURL_HANDLER_H
#define _CURL_HANDLER_H

#include <string>
#include <optional>

#include <curl/curl.h>

class CurlHandler
{
public:
  long responseCode;
  std::string responseData;

  CurlHandler(const std::string& url);
  ~CurlHandler(void);

  long get(const std::string& path);
  long post(
    const std::string& path,
    const std::optional<std::string>& data = std::nullopt,
    const std::string& query = "");

private:
  CURL* curl;
  const std::string baseUrl;

  long execute(const std::string& url);
  static size_t writeCallback(const char* ptr, size_t size, size_t nmemb, std::string* output);
};

#endif
