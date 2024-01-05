#ifndef _HWN_H_
#define _HWN_H_

#include "../GameBase.h"

class HWN: public GameBase
{
private:
  const std::string gameName = "Halloween";
  const std::string gamePath = "/game";
  const std::string tmpPath = "/game/tmp";
  const std::string auditsFile = "_game_audits.json";

public:
  const Json::Value processHighScores() override;
  uint32_t getGamesPlayed() override;

  const std::string& getGameName() override { return gameName; }
  const std::string& getGamePath() override { return gamePath; }
  const std::string& getTmpPath() override { return tmpPath; }
  const std::string& getAuditsFile() override { return auditsFile; }
};

#endif
