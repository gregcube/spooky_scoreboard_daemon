#!/bin/bash

# Require root access.
[[ "$(id -u)" -ne 0 ]] && { echo "Run as root to proceed" >&2; exit 1; }

main() {
  # Check params.
  [[ $# -eq 0 ]] && { echo "Missing game parameter" >&2; exit 1; }
  [[ "$1" != "hwn" && "$1" != "um" ]] && { echo "hwn or um" >&2; exit 1; }
  game=$1

  # Remount file system with read/write access.
  mount -o remount,rw /
  err "Failed to remount file system"

  # Check if wpa_supplicant file exists on USB drive.
  # Setup wifi access if file does exist.
  if [[ -f "/game/media/wpa_supplicant.pkg.tar.xz" ]]; then
    pacman -U --noconfirm /game/media/wpa_supplicant.pkg.tar.xz
    err "Failed to install wpa_supplicant"

    # Prompt for wifi ssid and password.
    read -p "Enter your WiFi SSID: " ssid
    read -p "Enter your WiFi pass: " pass

    # Write wifi configuration
    write_wifi_config "${ssid}" "${pass}"

    # Configure wpa_supplicant
    wpa_supplicant \
      -B \
      -D wext \
      -i wlp0s20u4u2 \
      -c /etc/wpa_supplicant/wifi.conf >/dev/null
    err "Failed to configure wifi interface"

    # Write systemd-networkd configuration for wireless interface.
    write_networkd "wlp0s20u4u2"
  else
    # Use ethernet if wpa_supplicant file does not exist.
    # Write systemd-networkd configuration for ethernet interface.
    write_networkd "enp1s0"
  fi

  # Write nameserver configuration.
  # Use google public DNS.
  write_resolvconf "8.8.8.8" "8.8.4.4"

  # Enable and start systemd-networkd.
  # Should hopefully bring up the network interface.
  systemctl enable systemd-networkd
  systemctl start systemd-networkd

  # Wait for an internet connection.
  echo "Waiting for internet connection"
  while ! ping -c 1 8.8.8.8 >/dev/null 2>&1; do echo -n .; sleep 1; done

  # Download and install libxpm package.
  # Required for spooky scoreboard to display QR codes on screen.
  curl -L -o /game/tmp/libxpm.pkg.tar.xz https://archive.archlinux.org/packages/l/libxpm/libxpm-3.5.12-2-x86_64.pkg.tar.xz
  err "Failed to download libxpm"
  pacman -U --noconfirm /game/tmp/libxpm.pkg.tar.xz
  err "Failed to install libxpm"

  # Download and install spooky scoreboard daemon.
  curl -L -o /game/tmp/ssbd.pkg.tar.xz https://github.com/gregcube/spooky_scoreboard_daemon/raw/master/dist/arch/spooky-scoreboard-daemon-0.0.2-1-x86_64.pkg.tar.xz
  err "Failed to download spooky scoreboard"
  pacman -U --noconfirm /game/tmp/ssbd.pkg.tar.xz
  err "Failed to install spooky scoreboard daemon"

  # Generate startup script.
  write_ssb_startup "${game}"

  # Add wpa_supplicant call to startup script if we're using wifi.
  if [[ -n "$ssid" && -n "$pass" ]]; then
    sed -i '2i\wpa_supplicant -B -Dwext -iwlp0s20u4u2 -c/etc/wpa_supplicant/wifi.conf' /etc/X11/xinit/xinitrc.d/99-ssbd.sh
  fi

  # Cleanup
  rm /game/tmp/ssbd.pkg.tar.xz
  rm /game/tmp/libxpm.pkg.tar.xz

  echo "Completed"
  return 0
}

err() {
  [[ $? -ne 0 ]] && { echo "$1" >&2; exit 1; }
}

write_wifi_config() {
cat <<EOF >/etc/wpa_supplicant/wifi.conf
network={
  ssid="$1"
  psk="$2"
}
EOF
}

write_resolvconf() {
cat <<EOF >/etc/resolv.conf
nameserver $1
nameserver $2
EOF
}

write_networkd() {
cat <<EOF >/etc/systemd/network/ssbd.network
[Match]
Name=$1

[Network]
DHCP=yes
EOF
}

write_ssb_startup() {
cat <<EOF >/etc/X11/xinit/xinitrc.d/99-ssbd.sh
#!/bin/bash
find /tmp -name 'serverauth.*' -type f -exec cp {} /game/.Xauthority \;
sleep 5
ssbd -g ${game} -d >/game/tmp/ssbd.log
EOF
chmod +x /etc/X11/xinit/xinitrc.d/99-ssbd.sh
}

main "$@"; exit
