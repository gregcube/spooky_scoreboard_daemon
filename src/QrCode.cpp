#include <iostream>
#include <fstream>
#include <string>

#include "QrCode.h"

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
  std::ofstream xpm("/game/tmp/qrcode.xpm");

  if (!xpm.is_open()) {
    throw std::runtime_error("Unable to write QR code.");
  }
 
  xpm << ch->responseData;
  xpm.close(); 
}

