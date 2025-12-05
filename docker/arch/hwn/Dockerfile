# This is the oldest image available.
# Halloween runs an old, out-dated version of arch for some reason.
FROM archlinux/archlinux:base-20201020.6556

# Set the ALA mirror for June 1, 2017.
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
  mpfr \
  pkg-config \
  libffi \
  libx11 \
  fontconfig \
  libxft \
  libxpm \
  patch \
  zlib \
  perl

# Copy code from the build context.
COPY . /ssbd

# Prepare vcpkg dependencies.
RUN cd /ssbd/vcpkg && ./bootstrap-vcpkg.sh && ./vcpkg install

# Patch yaml-cpp.
# Otherwise we can't link it on arch linux.
# Likely because it's an older version of cmake.
RUN cd /ssbd/vcpkg_installed/x64-linux/share/yaml-cpp && patch -p0 < ../../../../docker/arch/hwn/yaml-cpp.patch

# Build spooky scoreboard daemon.
# Use vcpkg toolchain file to ensure all dependencies are found via vcpkg
RUN cd /ssbd && mkdir build && cd build && cmake .. -DUSE_VCPKG_CURL=OFF && make && strip ssbd

WORKDIR /ssbd/build
CMD ["/bin/bash"]
