#ifndef _ED_H
#define _ED_H

#include "../GameBase.h"

class ED: public GameBase
{
private:
  const std::string gameName = "Evil Dead";
  const std::string gamePath = "/game/audits";
  const std::string tmpPath = "/game/tmp";
  const std::string auditsFile = "_game_audits.json";
  const std::string scoresFile = "highscores.json";

public:
  const Json::Value processHighScores() override;
  uint32_t getGamesPlayed() override;

  const std::string& getGameName() override { return gameName; }
  const std::string& getGamePath() override { return gamePath; }
  const std::string& getTmpPath() override { return tmpPath; }
  const std::string& getAuditsFile() override { return auditsFile; }
  const std::string& getHighScoresFile() override { return scoresFile; }

  /**
   * @brief Overrides the base class method to send sway commands.
   *
   * @return An integer status code.
   *
   * @see GameBase::sendswaycmd
   */
  int sendswaycmd() override;
};

#endif
