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
  const std::string scoresFile = "highscores.tcm";

public:
  const Json::Value processHighScores() override;
  uint32_t getGamesPlayed() override;

  const std::string& getGameName() override { return gameName; }
  const std::string& getGamePath() override { return gamePath; }
  const std::string& getTmpPath() override { return tmpPath; }
  const std::string& getAuditsFile() override { return auditsFile; }
  const std::string& getHighScoresFile() override { return scoresFile; }

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
