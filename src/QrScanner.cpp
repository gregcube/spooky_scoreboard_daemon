#include <iostream>
#include <cstring>
#include <vector>
#include <thread>

#include <unistd.h>
#include <fcntl.h>
#include <json/json.h>

#include "main.h"
#include "QrScanner.h"

void QrScanner::scan()
{
  char buf[MAX_UUID_LEN + 1];
  std::vector<char> uuid;

  while (isRunning.load()) {
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(ttyQR, &readfds);
    FD_SET(pipes[0], &readfds);

    int fd = select(std::max(ttyQR, pipes[0]) + 1, &readfds, NULL, NULL, NULL);

    if (fd == -1) {
      std::cerr << "Failed to select file descriptor" << std::endl;
      break;
    }

    // Break loop if pipes[1] is written to.
    if (FD_ISSET(pipes[0], &readfds)) {
      break;
    }

    // Process qr code data.
    if (FD_ISSET(ttyQR, &readfds)) {
      memset(buf, 0, sizeof(buf));

      ssize_t n = read(ttyQR, buf, sizeof(buf));
      if (n < 0 || n != sizeof(buf)) continue;

      uuid.clear();
      uuid.assign(buf, buf + sizeof(buf) - 1);
      int position = buf[sizeof(buf) - 1] - '0';

      std::cout << "QR code detected." << std::endl;
      playerLogin(uuid, position);
      uuid.clear();

      std::this_thread::sleep_for(std::chrono::seconds(3));
    }
  }
}

bool QrScanner::isStarted(int fd)
{
  return fd >= 0 && fcntl(fd, F_GETFD) != -1;
}

void QrScanner::stop()
{
  // Signal to exit scan loop.
  if (isStarted(pipes[1]) && write(pipes[1], "x", 1) < 0) {
    std::cerr << "Failed to signal exit pipe" << std::endl;
  }

  if (scanThread.joinable()) {
    scanThread.join();
  }

  if (isStarted(ttyQR)) close(ttyQR);
  if (isStarted(pipes[0])) close(pipes[0]);
  if (isStarted(pipes[1])) close(pipes[1]);
}

void QrScanner::start()
{
  ttyQR = open(qrDevice, O_RDONLY);

  if (ttyQR < 0) {
    throw std::runtime_error("Cannot open QR scanner.");
  }

  if (pipe(pipes) == -1) {
    throw std::runtime_error("Cannot open FIFO pipes.");
  }

  std::cout << "QR scanner started." << std::endl;
  scanThread = std::thread(&QrScanner::scan, this);
}

