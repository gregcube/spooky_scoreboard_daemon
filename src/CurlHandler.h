#ifndef _CURL_HANDLER_H
#define _CURL_HANDLER_H

#include <string>

#include <curl/curl.h>

class CurlHandler
{
public:
  long responseCode;
  std::string responseData;

  CurlHandler(const std::string& baseUrl);
  ~CurlHandler(void);

  long get(const std::string& path);
  long post(const std::string& path, const std::string& data);

private:
  CURL *curl;
  const std::string baseUrl;

  long execute(const std::string& url);
  static size_t writeCallback(const char* ptr, size_t size, size_t nmemb, std::string* output);
};

#endif
