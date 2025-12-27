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
#include <sys/stat.h>

#include "main.h"
#include "QrCode.h"

using namespace std;

QrCode::QrCode(const shared_ptr<WebSocket>& ws) :
  webSocket(ws),
  qrCodePath(game->getTmpPath() + "/qrcode.xpm") {}

QrCode::~QrCode()
{
  struct stat buf;
  if (stat(qrCodePath.c_str(), &buf) == 0 && remove(qrCodePath.c_str()) != 0) {
    cerr << "Failed to delete " << qrCodePath << endl;
  }
}

future<void> QrCode::download()
{
  auto promise = make_shared<std::promise<void>>();
  auto future = promise->get_future();

  Json::Value req;
  req["path"] = "/api/v1/qr";
  req["method"] = "POST"; // todo: should use GET perhaps(?)

  webSocket->send(req, [this, promise](const Json::Value& response) {
    if (response["status"].asInt() == 200) {
      try {
        this->write(response["body"].asString());
        promise->set_value();
      }
      catch (const runtime_error& e) {
        cerr << "Failed to request QR code." << endl;
        promise->set_exception(make_exception_ptr(e));
      }
    }
  });

  return future;
}

void QrCode::write(const string& data)
{
  ofstream xpm(qrCodePath);

  if (!xpm.is_open()) {
    throw runtime_error("Unable to write QR code.");
  }
 
  xpm << data;
  xpm.close(); 
}

// vim: set ts=2 sw=2 expandtab:

