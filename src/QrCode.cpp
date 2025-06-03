#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>

#include "QrCode.h"

QrCode::~QrCode()
{
  if (std::remove("/tmp/qrcode.xpm") != 0) {
    std::cerr << "Failed to delete /tmp/qrcode.xpm" << std::endl;
  }
}

QrCode* QrCode::get()
{
  long rc;

  if ((rc = ch->post("/api/v1/qr")) != 200) {
    throw std::runtime_error("Unable to download QR code.");
  }

  return this;
}

void QrCode::write()
{
  std::ofstream xpm("/tmp/qrcode.xpm");

  if (!xpm.is_open()) {
    throw std::runtime_error("Unable to write QR code.");
  }
 
  xpm << ch->responseData;
  xpm.close(); 
}

