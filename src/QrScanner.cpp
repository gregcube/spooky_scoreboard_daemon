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

QrScanner::QrScanner(const char* qrdev) : qrDevice(qrdev) {}

QrScanner::~QrScanner()
{
  stop();
}

void QrScanner::start()
{
  ttyQR = open(qrDevice, O_RDONLY);
  if (ttyQR < 0) {
    throw std::runtime_error("Cannot open QR scanner.");
  }

  int flags = fcntl(ttyQR, F_GETFL, 0);
  fcntl(ttyQR, F_SETFL, flags | O_NONBLOCK);

  run = true;
  scanThread = std::thread(&QrScanner::scan, this);
  std::cout << "QR scanner started." << std::endl;
}

void QrScanner::stop()
{
  if (!run.exchange(false)) return;

  cv.notify_one();
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

    timeval timeout{1, 0};
    int rc = select(ttyQR + 1, &readfds, nullptr, nullptr, &timeout);

    if (rc <= 0) continue;
    if (!FD_ISSET(ttyQR, &readfds)) continue;

    ssize_t n = read(ttyQR, buf, sizeof(buf));
    if (n != sizeof(buf)) {
      // Clear partial read.
      char discard[64];
      while (read(ttyQR, discard, sizeof(discard)) > 0);
      continue;
    }

    std::vector<char> uuid(buf, buf + MAX_UUID_LEN);
    int position = buf[MAX_UUID_LEN] - '0';

    std::cout << "QR code detected." << std::endl;
    playerHandler->login(uuid, position);

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

// vim: set ts=2 sw=2 expandtab:

