What you'll need
----------------

1. USB drive/ stick
2. USB keyboard
3. Ethernet cable or a USB WiFi adapter (see GAMENOTES.md)
4. Basic Linux know-how


Halloween CE (UP) Installation
------------------------------

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

