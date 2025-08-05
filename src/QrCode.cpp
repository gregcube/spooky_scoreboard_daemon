// Spooky Scoreboard Daemon
// Copyright (C) 2025 Greg MacKenzie
// https://spookyscoreboard.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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

