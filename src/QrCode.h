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
   * @brief Constructs a QrCode object.
   */
  QrCode();

  /**
   * @brief Destructor that deletes the temporary QR code file.
   */
  ~QrCode();

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

  /**
   * @brief Returns file system path to QR code.
   *
   * @return Path to QR code (in qrCodePath).
   */
  const std::string getPath() { return qrCodePath; }


private:
  const std::unique_ptr<CurlHandler> ch;
  const std::string qrCodePath;
};

#endif
