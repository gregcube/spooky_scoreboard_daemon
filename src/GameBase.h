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

#include <string>
#include <memory>
#include <functional>

#include <X11/Xlib.h>
#include <json/json.h>

#include "WebSocket.h"

class GameBase
{
protected:
  const std::string gameName;
  const std::string gamePath;
  const std::string scoresPath;
  const std::string tmpPath;
  const std::string highScoresFile;
  const std::string lastScoresFile;
  const std::string auditsFile;

public:
  GameBase(
    const std::string& name,
    const std::string& gPath,
    const std::string& sPath,
    const std::string& tPath,
    const std::string& hsFile,
    const std::string& lsFile,
    const std::string& aFile
  ) : gameName(name),
      gamePath(gPath),
      scoresPath(sPath),
      tmpPath(tPath),
      highScoresFile(hsFile),
      lastScoresFile(lsFile),
      auditsFile(aFile) {}

  virtual ~GameBase() = default;

  static std::unique_ptr<GameBase> create(const std::string& gameName);

  enum class ScoreType { High, Last, Mode };
  void uploadScores(const Json::Value& scores, ScoreType type);

  virtual uint32_t getGamesPlayed() = 0;

  /**
   * @brief Process highscores.
   *
   * The derived game class must override this function, returning a JSON
   * representation of highscores.
   *
   * @return A JSON object containing highscores.
   */
  virtual const Json::Value processHighScores() = 0;

  /**
   * @brief Process last game scores.
   *
   * The derived game class must override this function, returning a JSON
   * representation of the last game scores.
   *
   * @return A JSON object containing last game scores.
   */
  virtual const Json::Value processLastGameScores() = 0;

  /**
   * @brief Retrieves the game name.
   *
   * @return A constant reference to a string containing the game name.
   */
  const std::string& getGameName() const { return gameName; }

  /**
   * @brief Retrieves a file system path to the main game directory.
   *
   * @return A constant reference to a string containing a file system path.
   */
  const std::string& getGamePath() const { return gamePath; }

  /**
   * @brief Retrieves a file system path to where highscores are saved.
   *
   * @return A constant reference to a string containing a file system path.
   */
  const std::string& getScoresPath() const { return scoresPath; }

  /**
   * @brief Retrieves a file system path for temporary files.
   *
   * @return A constant reference to a string containing a file system path.
   */
  const std::string& getTmpPath() const { return tmpPath; }

  /**
   * @brief Retrieves the filename for game audits.
   *
   * @return A constant reference to a string containing the filename.
   */
  const std::string& getAuditsFile() const { return auditsFile; }

  /**
   * @brief Retrieves the highscores filename.
   *
   * @return A filename containing high scores.
   */
  const std::string& getHighScoresFile() const { return highScoresFile; }

  /**
   * @brief Retrieves the filename where last game scores are stored.
   *
   * @return A filename containing last game scores.
   *
   */
  const std::string& getLastScoresFile() const { return lastScoresFile; }

  /**
   * @brief Sends configuration commands to the game's window manager.
   *
   * Subclasses override this to handle game-specific window managers
   * (e.g. i3 or sway) for tasks like floating windows or setting
   * focus.
   *
   * @return 0 on success, negative value on failure.
   */
  virtual int sendWindowCommands() { return 0; }
};

using GameFactoryFunction = std::function<std::unique_ptr<GameBase>()>;
extern std::map<std::string, GameFactoryFunction> gameFactories;

// vim: set ts=2 sw=2 expandtab:

