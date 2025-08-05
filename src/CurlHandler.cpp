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
#include <string>

#include "main.h"
#include "CurlHandler.h"

CurlHandler::CurlHandler(const std::string& url) : baseUrl(url)
{
  curl = curl_easy_init();
  if (!curl) throw std::runtime_error("Unable to initialize CURL");
}

CurlHandler::~CurlHandler()
{
  if (curl) curl_easy_cleanup(curl);
}

long CurlHandler::get(const std::string& path)
{
  return execute(baseUrl + path);
}

long CurlHandler::post(const std::string& path, const std::optional<std::string>& data)
{
  std::string post = data.value_or("");

  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.c_str());

  return execute(baseUrl + path);
}

long CurlHandler::execute(const std::string& endpoint)
{
  CURLcode cc;
  struct curl_slist* hdrs = nullptr;

#ifdef DEBUG
  std::cout << "Calling " << endpoint << std::endl;
#endif

  hdrs = curl_slist_append(hdrs, "Content-Type: application/json; charset=utf-8");

  if (!machineId.empty() && !token.empty()) {
    std::string authHeader = "Authorization: Bearer " + token;
    std::string uuidHeader = "X-Machine-Uuid: " + machineId;
    hdrs = curl_slist_append(hdrs, authHeader.c_str());
    hdrs = curl_slist_append(hdrs, uuidHeader.c_str());
  }

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
  curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
  curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
  curl_easy_setopt(curl, CURLOPT_MAXCONNECTS, 5L);
#ifdef DEBUG
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
#endif

  responseData.erase();

  if ((cc = curl_easy_perform(curl)) != CURLE_OK) {
    std::cerr << "CURL failed: " << endpoint << ": " << curl_easy_strerror(cc) << std::endl;
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

  curl_slist_free_all(hdrs);
  hdrs = nullptr;

  return responseCode;
}

size_t CurlHandler::writeCallback(const char* ptr, size_t size, size_t nmemb, std::string* output)
{
  size_t total = size * nmemb;
  output->append(ptr, total);
  return total;
}

