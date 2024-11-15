#include <iostream>
#include <cstring>
#include <vector>
#include <thread>

#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "QrScanner.h"

const char QrScanner::keycodeToAscii(unsigned char keycode)
{
  // a-z
  if (keycode >= 4 && keycode <= 29) {
    return 'a' + (keycode - 4);
  }

  // 1-9
  if (keycode >= 30 && keycode <= 38) {
    return '1' + (keycode - 30);
  }

  // Other keycodes
  // We only care about '0' and '-'
  switch (keycode) {
  case 39:
    return '0';

  case 45:
    return '-';

  default:
    return 0;
  }
}

void QrScanner::scan()
{
  char buf[8];
  std::vector<char> uuid;
  const auto ch = std::make_unique<CurlHandler>(BASE_URL);
  auto last_scan = std::chrono::steady_clock::now();

  while (isRunning.load()) {
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(hid, &readfds);
    FD_SET(pipes[0], &readfds);

    int fd = select(std::max(hid, pipes[0]) + 1, &readfds, NULL, NULL, NULL);

    if (fd == -1) {
      std::cerr << "Failed to select file descriptor" << std::endl;
      break;
    }

    // Break loop if pipes[1] is written to.
    if (FD_ISSET(pipes[0], &readfds)) {
      break;
    }

    // Clear uuid if we've been idle more than 3 seconds.
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_scan).count() > 3 && !uuid.empty()) {
      std::cout << "Clearing uuid data." << std::endl;
      uuid.clear();
    }

    // Process qr code data.
    if (FD_ISSET(hid, &readfds)) {
      memset(buf, 0, sizeof(buf));

      ssize_t n = read(hid, buf, sizeof(buf));
      if (n < 0) continue;

      // First byte is a shift key modifier -- skip it.
      for (int i = 1; i < n; ++i) {
        char ascii = keycodeToAscii(buf[i]);
        if (ascii) uuid.push_back(ascii);
      }

      // +1 for requested player position.
      if (uuid.size() == MAX_UUID_LEN + 1) {
        uuid.resize(MAX_UUID_LEN + 2);

        std::cout << "QR code detected." << std::endl;

        int position = uuid[uuid.size() - 2] - '0';
        if (position >= 1 && position <= 4 && playerList.player[position - 1].empty()) {
          std::cout << "Logging in..." << std::endl;

          int rc = ch->post("/spooky/qrlogin", std::string(mid) + std::string(uuid.data()));
          if (rc != 200) continue;

          addPlayer(ch->responseData.c_str(), position);
        }
        else if (!playerList.player[position - 1].empty()) {
          std::cout << "Show player list" << std::endl;
          showPlayerList();
        }

        uuid.clear();
        std::this_thread::sleep_for(std::chrono::seconds(3));
      }
    }

    last_scan = now;
  }
}

void QrScanner::stop()
{
  // Signal to exit scan loop.
  if (write(pipes[1], "x", 1) < 0) {
    std::cerr << "Failed to signal exit pipe" << std::endl;
  }

  if (scanThread.joinable()) {
    scanThread.join();
  }

  close(hid);
  close(pipes[0]);
  close(pipes[1]);
}

void QrScanner::start()
{
  hid = open(qrDevice, O_RDONLY);

  if (hid < 0) {
    throw std::runtime_error("Cannot open QR scanner");
  }

  if (pipe(pipes) == -1) {
    throw std::runtime_error("Cannot open FIFO pipes");
  }

  std::cout << "QR scanner started." << std::endl;
  scanThread = std::thread(&QrScanner::scan, this);
}

