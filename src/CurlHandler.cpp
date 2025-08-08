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

using namespace std;

CurlHandler::CurlHandler(const string& url) : baseUrl(url)
{
  curl = curl_easy_init();
  if (!curl) throw runtime_error("Unable to initialize CURL");
}

CurlHandler::~CurlHandler()
{
  if (curl) curl_easy_cleanup(curl);
}

long CurlHandler::get(const string& path)
{
  return execute(baseUrl + path);
}

long CurlHandler::post(const string& path, const optional<string>& data, const string& query)
{
  string post = data.value_or("");
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.c_str());

  string fullUrl = baseUrl + path;
  if (!query.empty()) fullUrl += "?" + query;

  return execute(fullUrl);
}

long CurlHandler::execute(const string& endpoint)
{
  CURLcode cc;
  struct curl_slist* hdrs = nullptr;

#ifdef DEBUG
  cout << "Calling " << endpoint << endl;
#endif

  hdrs = curl_slist_append(hdrs, "Content-Type: application/json; charset=utf-8");

  if (!machineId.empty() && !token.empty()) {
    string authHeader = "Authorization: Bearer " + token;
    string uuidHeader = "X-Machine-Uuid: " + machineId;
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
    cerr << "CURL failed: " << endpoint << ": " << curl_easy_strerror(cc) << endl;
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

  curl_slist_free_all(hdrs);
  hdrs = nullptr;

  return responseCode;
}

size_t CurlHandler::writeCallback(const char* ptr, size_t size, size_t nmemb, string* output)
{
  size_t total = size * nmemb;
  output->append(ptr, total);
  return total;
}

