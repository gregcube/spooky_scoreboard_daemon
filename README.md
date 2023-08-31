Spooky Scoreboard Daemon
========================

Connect your Spooky Pinball machine to the Internet and join the
[online leaderboards](https://scoreboard.web.net).

Before we begin
---------------

I have only tested this on a Spooky Halloween CE pinball machine with the "UP"
computer board.  It has not been tested on the mini-PC that was used in later
Halloween builds.  Though it is likely to work the installation instructions
or install script may need to differ slightly.  I'm not sure how other Spooky
games work, but please open an issue thread if you own one and would like
to test.

What you'll need
----------------

1. USB drive/ stick
2. USB keyboard
3. Ethernet cable
4. Basic Linux know-how

Installation
------------

1. Download [Arch Linux](https://archlinux.org/download/) and flash the ISO to
   a USB drive/ stick.

2. On your pinball machine.  Open speaker panel to gain access to the computer.
   Connect ethernet, keyboard, and plug in the USB drive to an available USB
   port.  This assumes your ethernet connection works and is DHCP-enabled.

3. Turn your machine on.  It should auto boot from the USB drive.

4. Once fully booted execute the following commands at the terminal prompt:

    1. `mkdir /game3`
    2. `mount /dev/mmcblk0p3 /game3`
    3. `curl -o /game3/install.sh https://scoreboard.web.net/install.sh`
    4. `chmod +x /game3/install.sh`
    5. `/game3/install.sh`

5. Sign-up at https://scoreboard.web.net and follow the instructions to
   register your machine.  You will be given a 4-digit alphanumeric code.

6. Back to the terminal:

    6. `/game3/ssbd -r <CODE> >/game3/.ssbd_mid`

7. If no errors are reported you should be good.  Verify **/game3/.ssbd_mid**
   has your unique machine ID.  It's a 36-digit alphanumeric string.
   Keep this ID a secret.  Confirm by executing:

    7. `cat /game3/.ssbd_mid`

8. Proceed at the terminal if everything checks out.

    8. `umount /game3`
    9. `shutdown -h now`

9. Turn off your machine.  Unplug USB drive/ stick.  Disconnect keyboard.

10. Start your machine, play a game, and hope for the best.  Once you finish
    a game the scores should be uploaded to the server.

Note
----
https://scoreboard.web.net is currently invite only.  Send me a message or
create an issue thread and I'll get you setup.
