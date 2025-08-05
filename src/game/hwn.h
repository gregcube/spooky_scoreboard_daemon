#ifndef _HWN_H
#define _HWN_H

#include "../GameBase.h"

class HWN: public GameBase
{
private:
  const std::string gameName = "Halloween";
  const std::string gamePath = "/game";
  const std::string tmpPath = "/game/tmp";
  const std::string auditsFile = "_game_audits.json";
  const std::string highScoresFile = "highscores.config";
  const std::string lastScoresFile = "highscores.config";

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

#endif
