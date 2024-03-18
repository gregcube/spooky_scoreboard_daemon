What is this?
-------------

This is the installation instructions for installing Spooky Scoreboard Daemon
on a Halloween or Ultraman pinball machine running the "UP" computer board.

What you'll need
----------------

1. USB keyboard
2. Ethernet cable or a USB WiFi adapter (see GAMENOTES.md for compatibility)
3. USB drive/ stick
4. Basic Linux know-how

Halloween/ Ultraman (UP) Installation
---------------------------

0. Copy scripts/install-UP-HWN_UM.sh to a USB drive.

   Optionally download and copy the wpa_supplicant package
   if you plan to use wifi. Rename the file to `wpa_supplicant.pkg.tar.gz`.

1. At your pinball machine.
   Open the speaker panel to gain access to the computer.
   Connect ethernet or WiFi adapter, and USB drive.
   Connect keyboard to the USB port inside the coin door.
   Close the speaker panel so you can see the screen.

2. Turn your machine on. Wait for it to fully boot.

3. On the USB keyboard press `CTRL` + `ALT` + `F2`

4. At the `pinix login` prompt.
   Login using `pinball` as the username and password.

5. Run the following commands: `sudo bash /game/media/install-UP-HWN_UM.sh`
   When prompted enter `pinball` as the password.
   You will be prompted for your SSID and password if you're using wifi.

6. If the install script returns "Completed" then we should be good.
   Sign-up at https://scoreboard.web.net and follow the instructions to
   register your machine.  You will be given a 4-digit alphanumeric code.
   See Note below.

7. Back to the terminal:

   `ssbd -r <CODE> >/.ssbd_mid`

8. If no errors are reported you should be good.  Verify **/.ssbd_mid**
   has your unique machine ID.  It's a 36-digit alphanumeric string.
   Keep this ID a secret.  Confirm by executing:

   `cat /.ssbd_mid`

9. Proceed at the terminal if everything checks out.

   ```
   umount /game/media
   shutdown -h now
   ```

10. Turn off your machine.  Unplug USB drive/ stick.  Disconnect keyboard.

11. Start your machine, play a game, and hope for the best.  Once you finish
    a game the scores should be uploaded to the server.

