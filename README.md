Spooky Scoreboard Daemon
========================

Connect your Spooky Pinball machine to the Internet and join the
online leaderboards.

Before we begin
---------------
I have only tested this on a Spooky Halloween CE pinball machine with the "UP"
computer board.  It has not been tested on the mini-PC that was used in later
Halloween builds.  Though it is likely to work the installation instructions
may slightly differ.  If other Spooky games follow the same software
architecture then this should work as well.

What you'll need
----------------
1. USB drive/ stick
2. USB keyboard
3. Ethernet cable
4. Some Linux know-how

Installation
------------

1. Download [Arch Linux](https://archlinux.org/download/) and flash the ISO to
   a USB drive/ stick.

2. On your pinball machine.  Open speaker panel to gain access to the computer.
   Connect ethernet, keyboard, and plug in the USB drive to an available USB
   port.  This assumes your ethernet connection works and is DHCP-enabled.

3. Turn machine on.  It should auto boot from the USB drive.

4. Once fully booted execute the following commands at the terminal prompt:

    1. `mkdir /game3`
    2. `mount /dev/mmcblk0p3 /game3`
    3. `curl -O --output-dir /game3 https://scoreboard.web.net/ssbd.c`
    4. `curl -O --output-dir /game3 https://scoreboard.web.net/ssb.sh`
    5. `chmod +x /game3/ssb.sh`

5. Edit **/game3/ssb.sh** replacing the IP addresses to match your network.

6. Back to the terminal:

    6. `chroot /game3 gcc -O3 -o ssbd ssbd.c -lcurl`

7. Sign-up at https://scoreboard.web.net and follow the instructions to
   register your machine.  You will be given a 4-digit alphanumeric code.

8. Back to the terminal:

    7. `/game3/ssbd -r <code> > /game3/.ssbd_mid`

9. If no errors are reported you should be good.  Verify **/game3/.ssbd_mid**
   has your unique machine id.  It's a 36-digit alphanumeric string.
   Keep this ID a secret.  Confirm by executing:

    8. `cat /game3/.ssbd_mid`

10. Edit **/game3/etc/X11/xinit/xinitrc** and add the following line right before
    "./main.x86_64": /ssb.sh

    The last two lines should read:
    ```
    /ssb.sh  
    ./main.x86_64
    ```

11. Back to the terminal:

    9. `shutdown -h now`

12. Turn off your machine.  Unplug USB drive/ stick.  Disconnect keyboard.

13. Start your machine, play a game, and hope for the best.  Once you finish
    a game the scores should be uploaded to the server.
