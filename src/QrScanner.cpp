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
#include <cstring>
#include <vector>
#include <thread>

#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "QrScanner.h"

QrScanner::QrScanner(const char* qrdev) : qrDevice(qrdev) {
  if (pipe(wakePipe) == -1) {
    throw std::runtime_error("Failed to create wake pipe.");
  }
}

QrScanner::~QrScanner()
{
  stop();
  if (wakePipe[0] >= 0) close(wakePipe[0]);
  if (wakePipe[1] >= 0) close(wakePipe[1]);
}

void QrScanner::start()
{
  ttyQR = open(qrDevice, O_RDONLY);
  if (ttyQR < 0) {
    throw std::runtime_error("Cannot open QR scanner.");
  }

  run = true;
  scanThread = std::thread(&QrScanner::scan, this);
  std::cout << "QR scanner started." << std::endl;
}

void QrScanner::stop()
{
  if (!run.exchange(false)) return;
  if (wakePipe[1] >= 0) write(wakePipe[1], "", 1);
  if (scanThread.joinable()) scanThread.join();
  if (ttyQR >= 0) {
    close(ttyQR);
    ttyQR = -1;
  }
}

void QrScanner::scan()
{
  char buf[MAX_UUID_LEN + 1];

  while (run.load()) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(ttyQR, &readfds);
    FD_SET(wakePipe[0], &readfds);

    int rc = select(std::max(ttyQR, wakePipe[0]) + 1, &readfds, nullptr, nullptr, nullptr);
    if (rc < 0) break;

    // Break loop if wakePipe[1] is written to.
    if (FD_ISSET(wakePipe[0], &readfds)) break;

    // Data not ready on QR device, loop to next select.
    if (!FD_ISSET(ttyQR, &readfds)) continue;

    // Read QR data.
    memset(buf, 0, sizeof(buf));
    ssize_t n = read(ttyQR, buf, sizeof(buf));
    if (n < 0 || n != sizeof(buf)) continue;

    std::vector<char> uuid(buf, buf + MAX_UUID_LEN);
    int position = buf[MAX_UUID_LEN] - '0';

    std::cout << "QR code detected." << std::endl;
    playerHandler->login(uuid, position);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

// vim: set ts=2 sw=2 expandtab:

