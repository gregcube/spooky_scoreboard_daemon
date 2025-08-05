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

#ifndef _TCM_H
#define _TCM_H

#include "../GameBase.h"

class TCM: public GameBase
{
private:
  const std::string gameName = "Texas Chainsaw Massacre";
  const std::string gamePath = "/game/audits";
  const std::string tmpPath = "/game/tmp";
  const std::string auditsFile = "_game_audits.json";
  const std::string highScoresFile = "highscores.tcm";
  const std::string lastScoresFile = "highscores.tcm";

public:
  const Json::Value processHighScores() override;
  const Json::Value processLastGameScores() override;
  uint32_t getGamesPlayed() override;

  const std::string& getGameName() override { return gameName; }
  const std::string& getGamePath() override { return gamePath; }
  const std::string& getTmpPath() override { return tmpPath; }
  const std::string& getAuditsFile() override { return auditsFile; }
  const std::string& getHighScoresFile() override { return highScoresFile; }
  const std::string& getLastScoresFile() override { return lastScoresFile; }

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

#endif
