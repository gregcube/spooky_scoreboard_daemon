#ifndef _QR_CODE_H
#define _QR_CODE_H

#include <memory>
#include <cstddef>

#include "main.h"
#include "CurlHandler.h"

class QrCode
{
public:
  /**
   * @brief Constructs a QrCode object for the specified machine id.
   *
   * @param ptr Points to a C-string containing the machine id.
   */
  QrCode(const char* ptr) : ch(std::make_unique<CurlHandler>(BASE_URL)), mid(ptr) {}

  /**
   * @brief Fetches machine QR code from the server.
   *
   * @return A reference to this instance.
   */
  QrCode* get();

  /**
   * @brief Writes machine QR code to an output file stream.
   */
  void write();

private:
  const std::unique_ptr<CurlHandler> ch;
  const char* mid;
};

#endif
