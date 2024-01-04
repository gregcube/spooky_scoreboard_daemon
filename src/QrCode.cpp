#include <iostream>
#include <fstream>
#include <string>

#include "QrCode.h"

QrCode* QrCode::get(const char *ptr)
{
  std::cout << "Downloading QR Code" << std::endl;
  long rc;

  if ((rc = curlHandle->post("/spooky/qr", ptr)) != 200) {
    std::cerr << "QR: " << rc << std::endl;
    return nullptr;
  }

  return this;
}

void QrCode::write()
{
  std::ofstream xpm("/game/tmp/qrcode.xpm");

  if (!xpm.is_open()) {
    throw std::runtime_error("Unable to write QR code");
  }
 
  xpm << curlHandle->responseData;
  xpm.close(); 
}

