#ifndef _TNA_H_
#define _TNA_H_

#include "../GameBase.h"

class TNA : public GameBase
{
public:
  const std::string name() override;
  const std::string highScoresFile() override;
  const std::string getGamePath() override;
  const Json::Value processHighScores() override;
  uint32_t getGamesPlayed() override;
};

#endif
