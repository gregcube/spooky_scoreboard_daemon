#ifndef _HWN_H_
#define _HWN_H_

#include "../GameBase.h"

class HWN : public GameBase
{
public:
  const std::string tmpDir = "/game/tmp";

  const std::string name() override;
  const std::string highScoresFile() override;
  const std::string getGamePath() override;
  const Json::Value processHighScores() override;
  uint32_t getGamesPlayed() override;
};

#endif
