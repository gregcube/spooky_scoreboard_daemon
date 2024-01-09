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
and participating in leaderboards, or a private setting where only the operator
has access to top scores.

Players have the privilege to login to a machine and track their highscores.
Players may scan the on-screen QR code or manually access the machine login
page. Player scores are recorded and displayed, with the exception being if a
player profile is set to private.  All players are automatically logged out
after each game.


Notes
------

This has only been tested on a Spooky Halloween CE pinball machine
with the "UP" computer board.  It has not been tested on the mini-PC that
was used in later Halloween/ Ultraman builds.

See docs directory for more info.


Demo
----

[![Demo Video](https://img.youtube.com/vi/vLXcv0MFY6M/0.jpg)](https://www.youtube.com/watch?v=vLXcv0MFY6M)
