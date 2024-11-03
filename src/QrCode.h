#ifndef _QR_CODE_H
#define _QR_CODE_H

#include <memory>
#include <cstddef>

#include "CurlHandler.h"

class QrCode
{
public:
  /**
   * @brief Constructs a QrCode object with the specified curl handler.
   *
   * @param ch A shared pointer to a CurlHandler instance.
   */
  QrCode(const std::shared_ptr<CurlHandler>& ch) : curlHandle(ch) {};

  /**
   * @brief Fetches machine QR code from the server.
   *
   * @param ptr Points to a C-string containing the machine id.
   * @return A reference to this instance.
   */
  QrCode* get(const char* ptr);

  /**
   * @brief Writes machine QR code to an output file stream.
   */
  void write();

private:
  const std::shared_ptr<CurlHandler>& curlHandle;
};

#endif
