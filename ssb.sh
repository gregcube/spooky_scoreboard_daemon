#!/bin/bash
ip link set enp1s0 up
ip addr add 192.168.3.68/24 dev enp1s0
ip route add default via 192.168.3.1 dev enp1s0
/ssbd -fw
exit 0
