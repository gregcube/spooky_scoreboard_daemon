#ifndef _QR_SCANNER_H
#define _QR_SCANNER_H

#include <memory>

#include "CurlHandler.h"

class QrScanner
{
public:
  /**
   * @brief Construct a QrScanner instance.
   *
   * @param qrdev Pointer to a null-terminated string that specifies
   *              the path to the QR scanner device (e.g. "/dev/hidraw0").
   */
  QrScanner(const char *qrdev) : qrDevice(qrdev) {};

  /**
   * @brief Start the QR code scanner.
   */
  void start();

  /**
   * @brief Stop the QR code scanner.
   */
  void stop();

  /**
   * @brief Scan and process QR code data.
   *
   * This function runs in a dedicated thread and performs ongoing scans.
   * A player is logged into the machine if a valid user QR code is scanned.
   */
  void scan();

private:
  int hid;
  int pipes[2];
  const char *qrDevice;
  std::thread scanThread;
  const char keycodeToAscii(unsigned char keycode);
};

#endif 
