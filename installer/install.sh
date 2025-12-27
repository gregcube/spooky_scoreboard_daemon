#!/bin/bash

#
# Install script used in the Linux Live Kit installation image.
# Download installation image separately.
# See instructions in ../INSTALL.md
#

set -e
set -o pipefail

version="0.2.1-1"
echo "Starting Spooky Scoreboard Installer..."

cleanup() {
  echo "Cleaning up."
  if mountpoint -q /mnt/rootfs; then
    umount -q /mnt/rootfs/sys || true
    umount -q /mnt/rootfs/proc || true
    umount -q /mnt/rootfs/run || true
    umount -q /mnt/rootfs/dev || true
    umount -q /mnt/rootfs || true
  fi
}

trap cleanup EXIT

err() {
  echo "Error: $1" >&2
  exit 1
}

install() {
  echo "Installing for ${label}..."

  echo "Setting up network..."
  [[ "$game" == "ed" ]] && setup_wifi
  write_ssbd_network "$game"
  write_resolvconf
  systemctl enable systemd-networkd >/dev/null 2>&1
  systemctl start systemd-networkd >/dev/null 2>&1

  [[ -d /etc/wpa_supplicant ]] && {
    cp -r /etc/wpa_supplicant /mnt/rootfs/etc
    systemctl enable wpa_supplicant@wlp2s0 >/dev/null 2>&1
    systemctl start wpa_supplicant@wlp2s0 >/dev/null 2>&1
    systemctl restart systemd-networkd >/dev/null 2>&1
  }

  echo -n "Waiting for internet connection"
  until ping -c 1 8.8.8.8 >/dev/null 2>&1; do echo -n .; sleep 1; done
  echo " Connected."

  cp /etc/resolv.conf /mnt/rootfs/etc
  cp /etc/systemd/network/ssbd.network /mnt/rootfs/etc/systemd/network
  chroot /mnt/rootfs systemctl enable systemd-networkd >/dev/null 2>&1

  echo "Setting up QR scanner/reader..."
  setup_qr_scanner || err "Failed to find QR scanner/reader."

  echo "Installing dependencies..."
  case "$game" in
    hwn)
      for url in "${depends[@]}"; do
        file=$(basename "${url}")
        echo ">> Downloading ${file}..."
        curl -Ls --retry 10 -o "/tmp/${file}" "${url}" || err "Failed to download ${file}"
        cp "/tmp/${file}" /mnt/rootfs
        echo ">> Installing ${file}."
        chroot /mnt/rootfs pacman -U --noconfirm "/${file}" >/dev/null 2>&1 || err "Failed to install ${file}"
        rm "/mnt/rootfs/${file}"
      done
      ;;
    tcm)
      ;;
    ed)
      chroot /mnt/rootfs apt-get update >/dev/null 2>&1 || err "Failed to update apt packages"
      chroot /mnt/rootfs apt-get install -y "${depends[@]}" >/dev/null 2>&1 || err "Failed to install packages"
      chroot /mnt/rootfs systemctl enable wpa_supplicant@wlp2s0
      ;;
  esac

  echo "Installing spooky scoreboard daemon..."
  tar -xzf "./spooky_scoreboard_daemon/dist/${dist}/ssbd-${version}-${dist}-x86_64.tar.gz" -C /mnt/rootfs/usr/bin >/dev/null 2>&1 || err "Failed to install."
  chmod +x /mnt/rootfs/usr/bin/ssbd

  write_ssbd_service "$game"
  chroot /mnt/rootfs systemctl enable ssbd >/dev/null 2>&1

  echo "Enter your 4-digit registration code:"
  read -r code
  if ! [[ ${#code} -eq 4 && "$code" =~ ^[a-zA-Z0-9]{4}$ ]]; then
    err "Invalid code."
  fi

  ./ssbd -r ${code} -g ${game} -o /mnt/gamefs/.ssbd.json || err "Failed to register machine."
  echo "Completed."
}

update() {
  echo "Updating SSBd for ${label}..."
  ! [[ -f "/mnt/rootfs/usr/bin/ssbd" ]] && err "ssbd not installed."

  echo "Removing old version..."
  rm -f /mnt/rootfs/usr/bin/ssbd || true

  echo "Installing new version..."
  tar -xzf "./spooky_scoreboard_daemon/dist/${dist}/ssbd-${version}-${dist}-x86_64.tar.gz" -C /mnt/rootfs/usr/bin >/dev/null 2>&1 || err "Failed to install."
  chmod +x /mnt/rootfs/usr/bin/ssbd

  echo "Completed."
}

uninstall() {
  echo "Uninstalling SSBd for ${label}..."

  ! [[ -f "/mnt/rootfs/usr/bin/ssbd" ]] && err "ssbd not installed."

  chroot /mnt/rootfs systemctl disable ssbd >/dev/null 2>&1
  chroot /mnt/rootfs systemctl disable systemd-networkd >/dev/null 2>&1

  rm -f /mnt/rootfs/usr/bin/ssbd || true
  rm -f /mnt/rootfs/etc/systemd/system/ssbd.service || true
  rm -f /mnt/rootfs/etc/systemd/network/ssbd.network || true
  rm -f /mnt/rootfs/etc/udev/rules.d/99-ttyQR.rules || true
  rm -f /mnt/gamefs/.ssbd.json || true

  echo "Completed."
}

setup_qr_scanner() {
  local devices
  devices=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || true)
  [[ -z "$devices" ]] && return 1

  for dev in $devices; do
    vendor=$(udevadm info -q property -n "$dev" | grep "ID_VENDOR_ID" | cut -d= -f2)
    model=$(udevadm info -q property -n "$dev" | grep "ID_MODEL_ID" | cut -d= -f2)

    if [[ -n "$vendor" && -n "$model" ]] && is_qr_scanner "$vendor" "$model"; then
      write_udev_rule "$vendor" "$model"
      return 0
    fi
  done

  return 1
}

is_qr_scanner() {
  case "$1:$2" in
    152a:880f) return 0 ;; # symcode mj-390
    067b:2303) return 0 ;; # symcode mj-3300
    *) return 1 ;;
  esac
}

setup_wifi() {
  local ssid pass tmpfile
  tmpfile=$(mktemp)

  dialog --inputbox "WiFi SSID:" 8 40 2>"$tmpfile"
  ssid=$(<"$tmpfile")

  dialog --inputbox "WiFi Password:" 8 40 2>"$tmpfile"
  pass=$(<"$tmpfile")

  rm -f "$tmpfile"

  [[ -z "$ssid" || -z "$pass" ]] && err "SSID or password cannot be empty."

  mkdir -p /etc/wpa_supplicant

  cat <<EOF >/etc/wpa_supplicant/wpa_supplicant-wlp2s0.conf
network={
  ssid="$ssid"
  psk="$pass"
}
EOF
}

write_resolvconf() {
  cat <<EOF >/etc/resolv.conf
nameserver 8.8.8.8
nameserver 8.8.4.4
EOF
}

write_ssbd_network() {
  case "$1" in
    hwn|tcm) cat <<EOF >/etc/systemd/network/ssbd.network
[Match]
Name=enp1s0
Type=ether

[Network]
DHCP=yes
EOF
    ;;
    ed) cat <<EOF >/etc/systemd/network/ssbd.network
[Match]
Name=wlp2s0
Type=wlan

[Network]
DHCP=yes
EOF
    ;;
  esac
}

write_ssbd_service() {
  case "$1" in
    hwn) cat <<EOF >/mnt/rootfs/etc/systemd/system/ssbd.service
[Unit]
Description=Spooky Scoreboard Daemon (SSBd)
After=network-online.target
Wants=network-online.target

[Service]
Environment=HOME=/game
ExecStartPre=/usr/bin/find /tmp -name 'serverauth.*' -type f -exec cp {} /game/.Xauthority \;
ExecStart=/usr/bin/ssbd -g hwn
User=root
Group=root
Restart=on-failure
RestartSec=15

[Install]
WantedBy=multi-user.target
EOF
    ;;
    tcm) cat <<EOF >/mnt/rootfs/etc/systemd/system/ssbd.service
[Unit]
Description=Spooky Scoreboard Daemon (SSBd)
After=network-online.target
Wants=network-online.target

[Service]
ExecStartPre=/usr/bin/mkdir -p /game/tmp
ExecStartPre=/bin/sh -c 'while [ ! -e /run/user/0/i3/ipc-socket.* ]; do sleep 2; done'
ExecStartPre=/bin/sh -c 'echo "export I3_SOCKET_PATH=\$(ls /run/user/0/i3/ipc-socket.* 2>/dev/null)" > /game/tmp/.env'
ExecStart=/bin/sh -c '. /game/tmp/.env && exec /usr/bin/ssbd -g tcm'
User=root
Group=root
Restart=on-failure
RestartSec=15

[Install]
WantedBy=multi-user.target
EOF
    ;;
    ed) cat <<EOF >/mnt/rootfs/etc/systemd/system/ssbd.service
[Unit]
Description=Spooky Scoreboard Daemon (SSBd)
After=network-online.target
Wants=network-online.target

[Service]
ExecStartPre=/usr/bin/mkdir -p /game/tmp
ExecStartPre=/bin/sh -c 'while [ ! -e /run/user/1000/sway-ipc.1000.* ]; do sleep 2; done'
ExecStartPre=/bin/sh -c 'echo "export SWAYSOCK=\$(ls /run/user/1000/sway-ipc.1000.* 2>/dev/null)" > /game/tmp/.env'
ExecStart=/bin/sh -c '. /game/tmp/.env && exec /usr/bin/ssbd -g ed'
User=norville
Group=norville
Restart=on-failure
RestartSec=15

[Install]
WantedBy=multi-user.target
EOF
    ;;
  esac
}

write_udev_rule() {
  cat <<EOF >/mnt/rootfs/etc/udev/rules.d/99-ttyQR.rules
SUBSYSTEM=="tty", ATTRS{idVendor}=="$1", ATTRS{idProduct}=="$2", SYMLINK+="ttyQR"
EOF
}

# Identify game selection.
game=$(cut -d ' ' -f1- /proc/cmdline | sed -n 's/.*boot=\([^ ]*\).*/\1/p')
[[ -z "$game" ]] && err "Invalid selection."

mkdir -p /mnt/rootfs /mnt/gamefs
depends=()

case "$game" in
  hwn)
    label="Halloween"
    rootfs=/dev/mmcblk0p3
    gamefs=/dev/mmcblk0p4
    dist=arch
    depends+=("https://archive.archlinux.org/packages/l/libxpm/libxpm-3.5.12-2-x86_64.pkg.tar.xz")
    ;;
  tcm)
    label="Texas Chainsaw Massacre"
    rootfs=/dev/mmcblk0p2
    gamefs=/dev/mmcblk0p3
    dist=debian
    ;;
  ed)
    label="Evil Dead"
    rootfs=/dev/sda3
    gamefs=/dev/sda4
    dist=debian
    depends+=("wpasupplicant" "libnl-3-200" "libnl-genl-3-200" "libnl-route-3-200" "libpcsclite1")
    ;;
esac

tmpfile=$(mktemp)
dialog --menu "${label}" 20 40 10 \
  1 "Install" \
  2 "Update" \
  3 "Uninstall" \
  4 "Quit" 2>"$tmpfile"
opt=$(<"$tmpfile")
rm -f "$tmpfile"

mount $rootfs /mnt/rootfs || err "Failed to mount rootfs."
mount $gamefs /mnt/gamefs || err "Failed to mount gamefs."

# Setup chroot environment
mount -o bind /dev /mnt/rootfs/dev
mount -o bind /run /mnt/rootfs/run
mount -t proc /proc /mnt/rootfs/proc
mount -t sysfs /sys /mnt/rootfs/sys

case $opt in
  1) install ;;
  2) update ;;
  3) uninstall ;;
  4) echo "Quitting..." ;;
  *) err "Invalid choice." ;;
esac

exit 0

# vim: expandtab ts=2 sw=2

