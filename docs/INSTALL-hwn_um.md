What is this?
-------------

Instructions for installing Spooky Scoreboard Daemon on a Halloween or Ultraman pinball machine running the "UP" computer board.

What you'll need
----------------

1. USB keyboard
2. Ethernet cable or a USB WiFi adapter (see GAMENOTES.md for compatibility)
3. USB drive/ stick
4. Basic Linux know-how

Halloween/ Ultraman (UP) Installation
-------------------------------------

0. Copy `scripts/install-UP-HWN_UM.sh` from this repository to a USB drive.

   Optionally download the [wpa_supplicant](https://archive.archlinux.org/packages/w/wpa_supplicant/wpa_supplicant-2%3A2.8-1-x86_64.pkg.tar.xz) package if you plan to use wifi.  
   Rename the file to `wpa_supplicant.pkg.tar.xz` and copy to USB drive.

1. At your pinball machine:  
   a) Open the speaker panel to access the computer.  
   b) Connect ethernet or WiFi adapter, and USB drive.  
   c) Connect keyboard to the USB port inside the coin door.  
   d) Close the speaker panel so you can see the screen.  

2. Turn your machine on. Wait for it to fully boot.

3. On the keyboard press `CTRL` + `ALT` + `F2`

4. At the pinix login prompt. Login using `pinball` as the username and password.

5. Run the following command:  

   For Halloween:  
   `sudo bash /game/media/install-UP-HWN_UM.sh hwn`

   For Ultraman:  
   `sudo bash /game/media/install-UP-HWN_UM.sh um`
   
   When prompted enter `pinball` as the password.  
   You will be prompted for a SSID and password if you're using wifi.

6. If the install script returns "Completed" then we should be good.  
   Sign-up at https://scoreboard.web.net and follow the instructions to
   register your machine.  
   You will be given a 4-digit alphanumeric code.

7. Back to the terminal. Execute the following commands:
    ```
    sudo -s
    ssbd -r <CODE> >/.ssbd_mid
    ```

8. Verify **/.ssbd_mid** has your unique machine ID.  
    It's a 36-digit alphanumeric string. Keep this a secret.  
    Confirm with the command: `cat /.ssbd_mid`

9. Proceed if everything checks out:  
    ```
    umount /game/media
    shutdown -h now
    ```

10. Turn off your machine.  Unplug USB drive.  Disconnect keyboard.

11. Start your machine, play a game, and hope for the best.

