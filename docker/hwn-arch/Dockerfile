# Use the specified Arch Linux base image.
# This is the oldest image available.
FROM archlinux/archlinux:base-20201020.6556

# Set the ALA mirror for June 1, 2017.
# Halloween runs an old, out-dated version of arch.
RUN echo "Server = https://archive.archlinux.org/repos/2017/06/01/\$repo/os/\$arch" > /etc/pacman.d/mirrorlist

# Remove signature checks.
# Some packages fail verification, likely because they're old.
RUN sed -i '/^#*SigLevel/s/.*/SigLevel = Never/' /etc/pacman.conf

# Update package database.
RUN pacman -Syyu --noconfirm

# Install build tools and dependencies
RUN pacman -S --noconfirm \
  git \
  curl \
  zip \
  unzip \
  tar \
  cmake \
  ninja \
  gcc \
  make \
  cmake \
  mpfr \
  pkg-config \
  libffi \
  libx11 \
  fontconfig \
  libxft \
  libxpm

# Clone and compile spooky scoreboard daemon.
RUN git clone --recurse-submodules https://github.com/gregcube/spooky_scoreboard_daemon.git ssbd

# Temp. to test with docker builds.
RUN cd ssbd && git checkout docker

# Prepare vcpkg dependencies.
RUN cd ssbd/vcpkg && ./bootstrap-vcpkg.sh && ./vcpkg install yaml-cpp jsoncpp

# Build spooky scoreboard daemon.
RUN cd ssbd && mkdir build && cd build && cmake .. && make

WORKDIR /ssbd/build

CMD ["/bin/bash"]
