#!/bin/bash

# Find current IP address and gateway
IP_ADDR=$(ip -4 -o addr show enp1s0 | awk '{print $4}')
GATEWAY_IP=$(ip -4 route show default | awk '/default/ {print $3}')

if [ -z "$IP_ADDR" ] || [ -z "$GATEWAY_IP" ]; then
    echo "Failed to determine IP address or gateway."
    exit 1
fi

# Create and set up ssb.sh script
cat > /game3/ssb.sh <<EOF
#!/bin/bash
ip link set enp1s0 up
ip addr add $IP_ADDR dev enp1s0
ip route add default via $GATEWAY_IP dev enp1s0
/ssbd -md
exit 0
EOF
chmod +x /game3/ssb.sh

# Modify /game3/etc/resolv.conf
cat > /game3/etc/resolv.conf <<EOF
nameserver 8.8.8.8
nameserver 8.8.4.4
EOF

# Modify /game3/etc/X11/xinit/xinitrc
sed -i '/\.\/main\.x86_64$/i/ssb.sh' /game3/etc/X11/xinit/xinitrc

# Download and compile ssbd.c
curl -O --output-dir /game3 https://scoreboard.web.net/ssbd.c
chroot /game3 gcc -O3 -o ssbd ssbd.c -lcurl -lpthread

echo "Installation completed successfully."
