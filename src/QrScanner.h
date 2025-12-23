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

class QrScanner
{
public:
  /**
   * @brief Construct a QrScanner instance.
   *
   * @param qrdev Pointer to a null-terminated string that specifies
   *              the path to the QR scanner device.
   */
  explicit QrScanner(const char* qrdev);

  /**
   * @brief Deconstructor.
   */
  ~QrScanner();

  /**
   * @brief Start the QR code scanner.
   */
  void start();

  /**
   * @brief Stop the QR code scanner.
   */
  void stop();

private:
  /**
   * @brief Scan and process QR code data.
   *
   * This function runs in a dedicated thread and performs ongoing scans.
   * A player is logged into the machine if a valid user QR code is scanned.
   */
  void scan();

  const char* qrDevice;
  int ttyQR = -1;
  int wakePipe[2] = {-1, -1};

  std::atomic<bool> run{false};
  std::thread scanThread;
};

// vim: set ts=2 sw=2 expandtab:

