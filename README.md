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
spanning all supported games.

On the website you can register as a Player or an Operator or both.

Operators can register and administer their respective machines. They have the
option to make their machines publicly visible, thereby showcasing highscores
and participating in leaderboards, or a private setting where only the operator
has access to top scores.

Players can log into a machine via their player profile QR code.
Scan the on-screen QR code to easily access machine leaderboards.
Player scores are saved and displayed, with the exception being if a
player profile is set to private.
All players are automatically logged out after each game.


Supported Games
---------------

- Halloween (UP)
- Ultraman (UP)
- Texas Chainsaw Massacre (UP)
- Total Nuclear Annihilation (preliminary support)


Notes
------

You will need a USB QR code scanner/ reader. The following have been tested and work:
- [Symcode MJ-3300](https://amzn.to/4fuNqTx)
- [Symcode MJ-390](https://amzn.to/40QrH4D)*

Ensure the QR reader is set to USB-COM RS232 mode.

**Linux fails to recognize the device when connected via the USB extension cable that
is accessible inside the coin door. It does work however when connected directly
to the UP board, or when using a USB hub connected to the port inside the coin door; 
plug QR scanner into hub, and hub into USB port.*

This has only been tested on games with the "UP" computer board.
It has not been tested on the mini-PC that was used in later
Halloween/ Ultraman builds.

See docs directory for more info.


Demo
----

[![Demo Video](https://img.youtube.com/vi/hG7_vvCaeZU/0.jpg)](https://www.youtube.com/watch?v=hG7_vvCaeZU)
