#!/bin/bash

# Figure out if we're using wifi or ethernet.
while getopts ":ew" opt; do
  case $opt in
    e) connect="ethernet";;
    w) connect="wifi";;
    \?) echo "Invalid option: -$OPTARG" >&2
        exit 1;;
  esac
done

# Ensure -e or -w is used.
[ -n "$connect" ] || { echo "Use -e for ethernet, or -w for wifi." >&2; exit 1; }

generate_ssb_script() {
  cat <<EOF
#!/bin/bash
ip link set $1 up
ip addr add $2 dev $1
ip route add default via $3 dev $1
/ssbd -md 2>&1 >/game/tmp/ssbd.log
exit 0
EOF
}

# Setup chroot environment.
mount -o bind /dev /game3/dev
mount -o bind /run /game3/run
mount -t proc /proc /game3/proc
mount -t sysfs /sys /game3/sys

# Find device name.
dev=$(ip -4 -o addr show | awk '$2 != "lo" && $3 == "inet" {print $2}')
[ -n "$dev" ] || { echo "Failed to determine device name." >&2; exit 1; }

# Find current IP address and gateway.
ip=$(ip -4 -o addr show $dev | awk '{print $4}')
gw=$(ip -4 route show default | awk '/default/ {print $3}')
[ -z "$ip" ] || [ -z "$gw" ] && { echo "Failed to find IP address or gateway." >&2; exit 1; }

# Create ssb.sh script for wifi or ethernet.
if [ "$connect" = "wifi" ]; then
  mkdir /game3/etc/wpa_supplicant
  cp wifi.conf /game3/etc/wpa_supplicant
  generate_ssb_script "wlp0s20u3" "$ip" "$gw" > /game3/ssb.sh
  sed -i '2i\wpa_supplicant -B -Dwext -iwlp0s20u3 -c/etc/wpa_supplicant/wifi.conf' /game3/ssb.sh
else
  generate_ssb_script "$dev" "$ip" "$gw" > /game3/ssb.sh
fi

chmod +x /game3/ssb.sh

# Use Google for DNS resolver.
cat > /game3/etc/resolv.conf <<EOF
nameserver 8.8.8.8
nameserver 8.8.4.4
EOF

# Modify /game3/etc/X11/xinit/xinitrc
# Execute ssb.sh right before the game starts.
sed -i '/\.\/main\.x86_64$/i/ssb.sh' /game3/etc/X11/xinit/xinitrc

# WiFi needs wpa_supplicant installed.
# Use an older version because the host OS is dated.
if [ "$connect" = "wifi" ]; then
  curl -L -o /game3/wpa.tar.xz https://archive.archlinux.org/packages/w/wpa_supplicant/wpa_supplicant-2%3A2.8-1-x86_64.pkg.tar.xz
  [ $? -ne 0 ] && { echo "Failed to download wpa_supplicant." >&2; exit 1; }
  chroot /game3 pacman -U --noconfirm /wpa.tar.xz
fi

# Download and compile cJSON library.
# Needed for ssbd to parse game audit file.
curl -L -o /game3/cjson.tar.gz https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.16.tar.gz
[ $? -ne 0 ] && { echo "Failed to download cJSON." >&2; exit 1; }

tar -xzf /game3/cjson.tar.gz -C /game3
chroot /game3 /bin/bash -c "cd /cJSON-1.7.16 && make && exit"

# Download and compile ssbd.c
curl -O --output-dir /game3 https://scoreboard.web.net/ssbd.c
chroot /game3 gcc -O3 -I/cJSON-1.7.16 -o ssbd ssbd.c /cJSON-1.7.16/libcjson.a -lcurl -lpthread

# Cleanup
rm -rf /game3/cJSON-1.7.16
rm /game3/cjson.tar.gz
rm /game3/ssbd.c
[ "$connect" = "wifi" ] && rm /game3/wpa.tar.xz

umount /game3/sys
umount /game3/proc
umount /game3/run
umount /game3/dev

echo "Installation completed successfully."
