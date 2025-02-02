#!/bin/bash

# todo: add this to installer build.

cd /tmp
mkdir /mnt/usb
losetup -fP ssbd.img
lo=$(losetup -j ssbd.img | cut -f1 -d:)
part="${lo}p1"
echo "mkfs.vat -F 32 ${part}"
echo "mount -o loop ${part} /mnt/usb"
echo "unzip /tmp/spooky-scoreboard-installer-x86_64.zip -d /mnt/usb"
echo "cd /mnt/usb/spooky-scoreboard-installer/boot"
echo "./bootinst.sh"
echo "cd -"
echo "umount /mnt/usb"
losetup -d ${lo}
