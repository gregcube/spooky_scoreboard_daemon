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

class Halloween: public GameBase
{
private:
  inline static const std::string gameName = "Halloween";
  inline static const std::string gamePath = "/game";
  inline static const std::string tmpPath = "/game/tmp";
  inline static const std::string auditsFile = "_game_audits.json";
  inline static const std::string highScoresFile = "highscores.config";
  inline static const std::string lastScoresFile = "highscores.config";

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
};

