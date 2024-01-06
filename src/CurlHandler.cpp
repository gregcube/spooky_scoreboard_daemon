#include <iostream>
#include <string>

#include "CurlHandler.h"

CurlHandler::CurlHandler(const std::string& baseUrl) : baseUrl(baseUrl)
{
  curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Failed to init curl" << std::endl;
  }
}

CurlHandler::~CurlHandler()
{
  if (curl) {
    curl_easy_cleanup(curl);
  }
}

long CurlHandler::get(const std::string& path)
{
  return execute(baseUrl + path);
}

long CurlHandler::post(const std::string& path, const std::string& data)
{
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
  return execute(baseUrl + path);
}

long CurlHandler::execute(const std::string& endpoint)
{
  CURLcode cc;
  struct curl_slist *hdrs = NULL;

#ifdef DEBUG
  std::cout << "Calling " << endpoint << std::endl;
#endif

  hdrs = curl_slist_append(hdrs, "Content-Type: text/plain; charset=utf-8");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
  curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
#ifdef DEBUG
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
#endif

  responseData.erase();

  if ((cc = curl_easy_perform(curl)) != CURLE_OK) {
    std::cerr << "CURL failed: " << curl_easy_strerror(cc) << std::endl;
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
  return responseCode;
}

size_t CurlHandler::writeCallback(
  const char *ptr,
  size_t size,
  size_t nmemb,
  std::string *output
) {
  size_t total = size * nmemb;
  output->append(ptr, total);
  return total;
}
