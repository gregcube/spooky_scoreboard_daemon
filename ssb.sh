#!/bin/bash
ip link set enp1s0 up
ip addr add 192.168.1.2/24 dev enp1s0
ip route add default via 192.168.1.1 dev enp1s0
/ssbd -md
exit 0
