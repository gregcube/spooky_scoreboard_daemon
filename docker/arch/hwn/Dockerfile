# Halloween with the UP computer runs a very old version of archlinux from 2017.
# The archlinux image from 2017 is no longer available.
# Build with Fedora 26 for glibc 2.25 and other required, older libraries.
FROM fedora:26

# Update package database.
RUN dnf update -y

# Install build tools and dependencies.
RUN dnf install -y \
  git \
  curl \
  zip \
  unzip \
  tar \
  cmake \
  ninja-build \
  gcc \
  gcc-c++ \
  make \
  mpfr-devel \
  pkgconfig \
  libffi-devel \
  libX11-devel \
  fontconfig-devel \
  libXft-devel \
  libXpm-devel \
  patch \
  zlib-devel \
  perl \
  perl-IPC-Cmd \
  libuuid-devel

# Copy code from the build context.
COPY . /ssbd

# Prepare vcpkg dependencies.
RUN cd /ssbd/vcpkg && ./bootstrap-vcpkg.sh && ./vcpkg install

# Build spooky scoreboard daemon.
RUN cd /ssbd && mkdir build && cd build && cmake .. && make && strip ssbd

WORKDIR /ssbd/build
CMD ["/bin/bash"]
