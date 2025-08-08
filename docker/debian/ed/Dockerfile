FROM debian:bookworm

# Install build tools and dependencies.
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
  libx11-dev \
  libcurl4-openssl-dev \
  libfontconfig-dev \
  libxft-dev \
  libxpm-dev

# Copy code from the build context.
COPY . /ssbd

# Prepare vcpkg dependencies.
RUN cd /ssbd/vcpkg && ./bootstrap-vcpkg.sh && ./vcpkg install yaml-cpp jsoncpp

# Build spooky scoreboard daemon.
RUN cd /ssbd && mkdir build && cd build && cmake .. && make && strip ssbd

WORKDIR /ssbd/build
CMD ["/bin/bash"]
