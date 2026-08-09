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
#include <string>
#include <thread>
#include <algorithm>
#include <cctype>

#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "QrScanner.h"

// Signed QR login tokens (JWT) are ~200-300 bytes. HID/serial scanners
// typically append CR/LF after the payload.
static constexpr size_t MAX_QR_TOKEN_LEN = 1024;

QrScanner::QrScanner(const char* qrdev) : qrDevice(qrdev)
{
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

static std::string trimToken(std::string token)
{
  // Strip CR/LF and surrounding whitespace scanners often append.
  while (!token.empty() && (token.back() == '\n' || token.back() == '\r' || std::isspace(static_cast<unsigned char>(token.back())))) {
    token.pop_back();
  }

  size_t start = 0;
  while (start < token.size() && std::isspace(static_cast<unsigned char>(token[start]))) {
    ++start;
  }

  return token.substr(start);
}

void QrScanner::scan()
{
  char buf[256];
  std::string token;
  token.reserve(256);

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

    ssize_t n = read(ttyQR, buf, sizeof(buf));
    if (n <= 0) continue;

    for (ssize_t i = 0; i < n; ++i) {
      char c = buf[i];
      if (c == '\n' || c == '\r') {
        std::string payload = trimToken(std::move(token));
        token.clear();
        token.reserve(256);

        // JWT compact form has two dots (header.payload.sig).
        if (payload.size() < 20 || std::count(payload.begin(), payload.end(), '.') != 2) {
          continue;
        }

        std::cout << "QR code detected..." << std::endl;
        playerHandler->login(payload);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        continue;
      }

      if (token.size() >= MAX_QR_TOKEN_LEN) {
        // Overflow/ noise - discard and resync on next terminator.
        token.clear();
        continue;
      }

      token.push_back(c);
    }
  }
}

// vim: set ts=2 sw=2 expandtab:
