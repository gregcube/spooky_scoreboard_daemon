#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>

#include "main.h"
#include "QrCode.h"

using namespace std;

QrCode::QrCode() :
  ch(make_unique<CurlHandler>(BASE_URL)),
  qrCodePath(game->getTmpPath() + "/qrcode.xpm") {}

QrCode::~QrCode()
{
  if (remove(qrCodePath.c_str()) != 0) {
    cerr << "Failed to delete " << qrCodePath << endl;
  }
}

QrCode* QrCode::get()
{
  long rc;

  if ((rc = ch->post("/api/v1/qr")) != 200) {
    throw runtime_error("Unable to download QR code.");
  }

  return this;
}

void QrCode::write()
{
  ofstream xpm(qrCodePath);

  if (!xpm.is_open()) {
    throw runtime_error("Unable to write QR code.");
  }
 
  xpm << ch->responseData;
  xpm.close(); 
}

