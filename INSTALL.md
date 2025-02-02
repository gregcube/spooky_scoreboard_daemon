# Spooky Scoreboard Installation Instructions

## Requirements
- USB stick (2GB or larger)
- USB keyboard
- Wired (Ethernet) internet connection
- A compatible QR reader/scanner
- A supported Spooky pinball machine

**Tip**: Use a USB hub for connecting the QR scanner and keyboard. Connect the
hub to the USB port inside the coin door. If you do not have a hub, you can make
use of any extra USB ports on the computer.

#### Compatible QR readers
- [Symcode MJ-3300](https://amzn.to/4fuNqTx)
- [Symcode MJ-390](https://amzn.to/40QrH4D)*

**The system fails to recognize the device when connected via the USB extension cable that
is accessible inside the coin door. It does work however when connected directly
to the UP board, or when using a USB hub connected to the port inside the coin door; 
plug QR scanner into hub, and hub into USB port.*

#### Supported Games
Only games using the "UP" computer board are currently supported.
- Halloween
- Ultraman
- Texas Chainsaw Massacre

If your game has a mini-PC, please create an issue thread [here](https://github.com/gregcube/spooky_scoreboard_daemon/issues),
and we can work together to add support for your game.

---

## Installation Steps

0. [**Download the Spooky Scoreboard Installer**](https://scoreboard.web.net/ssbd-installer.zip)
   - Unzip the file.

1. **Prepare the USB drive**
   - Flash `ssbd-installer.img` to a USB stick using your favourite app.
   - Or use [balenaEtcher](https://etcher.balena.io/) (supports Windows, Linux, Mac).

2. **Setup Connections**
   - Open the speaker panel to gain access to the computer.
   - Plug USB stick into the computer.
   - Connect USB hub to the USB port inside the coin door.
   - Plug the QR reader and keyboard into the USB hub.
   - Close speaker panel.

3. **Power On**
   - Turn on your machine.

4. **System Installation**
   - Select your game at the boot prompt; press enter to continue.
   - Wait for the system to boot.
   - Select `Install` at the menu screen.
   - When prompted enter your 4-digit alphanumeric registration code.

5. **Obtain Your Registration Code**
   - Browse [here](https://scoreboard.web.net/user/register) to sign-up.
   - Once logged in, browse to `Register machine` or click [here](https://scoreboard.web.net/m/add).
   - Enter your game details; click `Save`.
   - Your 4-digit code will be displayed.

6. **Complete the Installation**
   - Power off your machine if the installer completes without error.
   - Unplug the keyboard and USB stick.

7. **Final Step**
   - Start up your machine as normal and hope for the best!

---

**Notes:**
- Ensure your wired internet connection is active during the installation.
- Please create an issue thread [here](https://github.com/gregcube/spooky_scoreboard_daemon/issues)
if you encounter any errors or other issues.

