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
  patch

# Clone and compile spooky scoreboard daemon.
RUN git clone --recurse-submodules https://github.com/gregcube/spooky_scoreboard_daemon.git ssbd

# Prepare vcpkg dependencies.
RUN cd ssbd/vcpkg && ./bootstrap-vcpkg.sh && ./vcpkg install yaml-cpp jsoncpp

# Patch yaml-cpp.
# Otherwise we can't link it on arch linux.
# Likely because it's an older version of cmake.
RUN cd ssbd/vcpkg/installed/x64-linux/share/yaml-cpp && patch -p0 < ../../../../../docker/arch/hwn/yaml-cpp.patch

# Build spooky scoreboard daemon.
RUN cd ssbd && mkdir build && cd build && cmake .. && make && strip ssbd

WORKDIR /ssbd/build
CMD ["/bin/bash"]
