FROM debian:bookworm

# Install build tools and dependencies.
# Note: curl package is built via vcpkg, not system packages
RUN apt update && apt install -y \
  git \
  curl \
  zip \
  unzip \
  tar  \
  cmake \
  gcc \
  make \
  build-essential \
  pkg-config \
  libssl-dev \
  zlib1g-dev \
  libx11-dev \
  libfontconfig-dev \
  libxft-dev \
  libxpm-dev

# Copy code from the build context.
COPY . /ssbd

# Prepare vcpkg dependencies.
RUN cd /ssbd/vcpkg && ./bootstrap-vcpkg.sh && ./vcpkg install

# Build spooky scoreboard daemon.
# Use vcpkg toolchain file to ensure all dependencies are found via vcpkg
RUN cd /ssbd && mkdir build && cd build && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=/ssbd/vcpkg/scripts/buildsystems/vcpkg.cmake && \
    make && strip ssbd

WORKDIR /ssbd/build
CMD ["/bin/bash"]
