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
