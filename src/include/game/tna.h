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

#ifndef _TNA_H
#define _TNA_H

#include "GameBase.h"

class TNA: public GameBase
{
private:
  const std::string gameName = "Total Nuclear Annihilation";
  const std::string tmpPath = "/tna/game/tmp";
  const std::string gamePath = "/tna/game/config";
  const std::string auditsFile = "tna.yaml";
  const std::string highScoresFile = "tna.yaml";
  const std::string lastScoresFile = "tna.yaml";
  const std::string logPath = "/tna/game/logs";
  const std::regex logFileRegex = std::regex(R"(tna_log_\d{8}_\d{6}\.txt)");

public:
  const Json::Value processHighScores() override;
  const Json::Value processLastGameScores() override;
  bool processAchievements() override;
  uint32_t getGamesPlayed() override;

  const std::string& getGameName() override { return gameName; }
  const std::string& getGamePath() override { return gamePath; }
  const std::string& getTmpPath() override { return tmpPath; }
  const std::string& getAuditsFile() override { return auditsFile; }
  const std::string& getHighScoresFile() override { return highScoresFile; }
  const std::string& getLastScoresFile() override { return lastScoresFile; }
  const std::string& getLogPath() override { return logPath; }
  const std::regex& getLogFileRegex() override { return logFileRegex; }
};

#endif
