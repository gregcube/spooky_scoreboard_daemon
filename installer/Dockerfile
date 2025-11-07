FROM debian:latest

RUN useradd -m -d /home/ssb ssb

# Enable non-free-firmware repository for firmware-realtek
RUN echo "deb http://deb.debian.org/debian trixie main non-free-firmware" > /etc/apt/sources.list && \
    echo "deb http://deb.debian.org/debian-security trixie-security main non-free-firmware" >> /etc/apt/sources.list && \
    echo "deb http://deb.debian.org/debian trixie-updates main non-free-firmware" >> /etc/apt/sources.list

RUN apt update && apt install -y \
    git \
    zip \
    make \
    cmake \
    g++ \
    parted \
    dialog \
    libxpm-dev \
    libxft-dev \
    libcurl4-openssl-dev \
    squashfs-tools \
    dosfstools \
    genisoimage \
    curl \
    xz-utils \
    linux-image-amd64 \
    util-linux \
    firmware-realtek \
    systemd \
    systemd-sysv \
    dbus \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /home/ssb

COPY ./ /home/ssb/spooky_scoreboard_daemon/

RUN ls -la /home/ssb

RUN cd spooky_scoreboard_daemon/vcpkg && \
       ./bootstrap-vcpkg.sh && \
       ./vcpkg install

RUN mkdir spooky_scoreboard_daemon/build && \
    cd spooky_scoreboard_daemon/build && \
    cmake .. && \
    make && \
    cp ssbd /home/ssb

RUN git clone https://github.com/Tomas-M/linux-live.git /linux-live

RUN cd spooky_scoreboard_daemon/installer && \
    cp config /linux-live && \
    cp syslinux.cfg /linux-live/bootfiles && \
    cp install.sh generate.sh ../../

RUN chmod +x install.sh generate.sh

RUN cd /linux-live && ./build && /tmp/gen_spooky-scoreboard-installer_zip.sh

RUN dd if=/dev/zero of=/tmp/ssbd.img count=1024 bs=1M status=progress && \
    cd /tmp && \
    parted ssbd.img --script mklabel gpt && \
    parted ssbd.img --script mkpart ESP fat32 0% 100% && \
    parted ssbd.img --script set 1 esp on

CMD ["./generate.sh"]
