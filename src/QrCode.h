#ifndef _QR_CODE_H
#define _QR_CODE_H

#include <memory>
#include <cstddef>
#include "CurlHandler.h"

class QrCode
{
public:
  QrCode(const std::shared_ptr<CurlHandler>& ch) : curlHandle(ch) {};
  QrCode* get(const char* ptr);
  void write();

private:
  const std::shared_ptr<CurlHandler>& curlHandle;
};

#endif
