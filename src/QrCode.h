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

#pragma once

#include <future>

#include "main.h"

class QrCode
{
public:
  /**
   * @brief Constructs a QrCode object.
   */
  QrCode(const std::shared_ptr<WebSocket>& ws);

  /**
   * @brief Destructor that deletes the temporary QR code file.
   */
  ~QrCode();

  /**
   * @brief Fetches machine QR code from the server.
   */
  std::future<void> download();

  /**
   * @brief Returns file system path to QR code.
   *
   * @return Path to QR code (in qrCodePath).
   */
  const std::string getPath() { return qrCodePath; }


private:
  const std::shared_ptr<WebSocket> webSocket;
  const std::string qrCodePath;

  /**
   * @brief Writes machine QR code to an output file stream.
   */
  void write(const std::string& data);
};


// vim: set ts=2 sw=2 expandtab:

