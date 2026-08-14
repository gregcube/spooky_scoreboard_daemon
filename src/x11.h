// Spooky Scoreboard Daemon
// Copyright (C) 2025 Greg MacKenzie
// https://spookyscoreboard.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <string>

#define TIMER_DEFAULT 15

void x11Init();
void drawWindow(int index);
void openWindows();
void closeWindows();
void startWindowThread(int index);

/**
 * Show a server notice on window 4.
 * timeoutSec <= 0 uses TIMER_DEFAULT. width/height <= 0 use the default 320x480.
 */
void showServerMessage(const std::string& text, int timeoutSec = 0, int width = 0, int height = 0);

// vim: set ts=2 sw=2 expandtab:

