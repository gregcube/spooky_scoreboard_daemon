Spooky Scoreboard Daemon
========================

Connect your Spooky Pinball machine to the Internet and join the
[online leaderboards](https://scoreboard.web.net).

What is it?
-----------

Spooky Scoreboard consists of both a web platform and this daemon.

The daemon runs in the background on your Spooky pinball machine, periodically
sending data to the web server. This data includes highscores and game audit
information. The server processes this data to display in a variety of formats.
This includes showcasing top scores specific to each machine and leaderboards
spanning all supported games. *Only Halloween is supported at this moment. More
on that below.*

On the website you can register as a Player or an Operator or both.

Operators can register and administer their respective machines. They have the
option to make their machines publicly visible, thereby showcasing highscores
and participating in leaderboards, or a private setting where only
the operator has access to top scores. To enhance user convenience, I recommend
generating a QR code and affixing it to your machine. This enables players to
effortlessly log in and engage with the system.

Players have the privilege to log in to a machine and track their highscores.
Players may scan a QR code or manually access the machine's login page. It's
important to note that a game must be initiated before players can log in.
That means press start (for 1-4 players), then login through the website to
claim your spot.  Player scores are recorded and displayed, with the exception
being if a player profile is set to private.  All players are automatically
logged out after each game.

See Note below regarding website access.

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
3. Ethernet cable or a USB WiFi adapter using the RTL8188EUS chipset
4. Basic Linux know-how

I use this WiFi adapter: [TP-Link (TL-WN725N) N150 Nano](https://www.amazon.ca/dp/B008IFXQFU?&_encoding=UTF8&tag=psha-20&linkCode=ur2&linkId=32f0cd11b4fa5909610f73548f409619&camp=15121&creative=330641)


Installation
------------

1. Download [Arch Linux](https://archlinux.org/download/) and flash the ISO to
   a USB drive/ stick.

2. At your pinball machine.  Open the speaker panel to gain access to the computer.
   Connect ethernet or WiFi adapter and USB drive.  Plug keyboard into the
   USB port inside the coin door.  The following assumes your Internet connection
   works and is DHCP-enabled.  Close the speaker panel so you can see the screen.

3. Turn your machine on.  It should auto boot from the USB drive.  Proceed
   once fully booted and you're at the `root@archiso ~ #` prompt.

4. If you're using WiFi we need to setup your Internet connection. Skip
   to step 5 if you're using ethernet.

   1. Type the following at the terminal prompt:
      
      `cat <<EOF > wifi.conf`

   2. Type the following at the `heredoc>` prompt. Replace &lt;SSID&gt; and
   &lt;PASS&gt; with your WiFi credentials.

      ```
      \`heredoc> network={
      \`heredoc>   ssid="<SSID>"
      \`heredoc>   psk="<PASS>"
      \`heredoc> }
      \'heredoc> EOF
      ```
   3. Type the following command to connect:
      
      `wpa_supplicant -B -i wlan0 -c wifi.conf`

   If DHCP is enabled you should be assigned an IP address and ready to proceed.
   
5. Execute the following commands at the terminal prompt:

    1. `mkdir /game3`
    2. `mount /dev/mmcblk0p3 /game3`
    3. `curl -o /game3/install.sh https://scoreboard.web.net/install.sh`
    4. `chmod +x /game3/install.sh`
    5. `/game3/install.sh -w` for WiFi, or `/game3/install.sh -e` for ethernet

6. Sign-up at https://scoreboard.web.net and follow the instructions to
   register your machine.  You will be given a 4-digit alphanumeric code.
   See Note below.

7. Back to the terminal:

    6. `/game3/ssbd -r <CODE> >/game3/.ssbd_mid`

8. If no errors are reported you should be good.  Verify **/game3/.ssbd_mid**
   has your unique machine ID.  It's a 36-digit alphanumeric string.
   Keep this ID a secret.  Confirm by executing:

    7. `cat /game3/.ssbd_mid`

9. Proceed at the terminal if everything checks out.

    8. `umount /game3`
    9. `shutdown -h now`

10. Turn off your machine.  Unplug USB drive/ stick.  Disconnect keyboard.

11. Start your machine, play a game, and hope for the best.  Once you finish
    a game the scores should be uploaded to the server.

Note
----
https://scoreboard.web.net is currently invite only.  Send me a message or
create an issue thread and I'll get you setup.
