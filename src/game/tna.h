#ifndef _TNA_H_
#define _TNA_H_

#include "../GameBase.h"

class TNA: public GameBase
{
private:
  const std::string gameName = "Total Nuclear Annihilation";
  const std::string tmpPath = "/tna/game/tmp";
  const std::string gamePath = "/tna/game/config";
  const std::string auditsFile = "tna.yaml";

public:
  const Json::Value processHighScores() override;
  uint32_t getGamesPlayed() override;

  const std::string& getGameName() override { return gameName; }
  const std::string& getGamePath() override { return gamePath; }
  const std::string& getTmpPath() override { return tmpPath; }
  const std::string& getAuditsFile() override { return auditsFile; }
};

#endif
