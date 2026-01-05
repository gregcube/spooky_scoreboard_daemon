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

#include "GameBase.h"

class TexasChainsawMassacre: public GameBase
{
public:
  TexasChainsawMassacre() : GameBase(
    "Texas Chainsaw Massacre",
    "/game",
    "/game/audits",
    "/game/tmp",
    "highscores.tcm",
    "highscores.tcm",
    "_game_audits.json"
  ) {}

  const Json::Value processHighScores() override;
  const Json::Value processLastGameScores() override;
  uint32_t getGamesPlayed() override;

  /**
   * @brief Overrides the base class method to send i3 commands.
   *
   * TCM uses the i3 window manager.
   * This function sends two commands, focus & floating enable, to ensure
   * the player login window is displayed on screen.
   *
   * @return An integer status code.
   *
   * @see GameBase::sendi3cmd
   */
  int sendi3cmd() override;
};

// vim: set ts=2 sw=2 expandtab:

