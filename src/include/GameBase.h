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

#ifndef _GAME_BASE_H
#define _GAME_BASE_H

#include <string>
#include <memory>
#include <functional>
#include <regex>

#include <X11/Xlib.h>
#include <json/json.h>

class GameBase
{
public:
  virtual ~GameBase() = default;

  static std::unique_ptr<GameBase> create(const std::string& gameName);

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
   * @brief Process achievements.
   *
   * The derived game class must override this function, processing
   * achievements for the game.  The achievements parameter is a JSON object
   * containing the current achievements state.  The function should update
   * this object with any new achievements unlocked during the last game.
   *
   * @param achievements A reference to a JSON object containing achievements.
   * @return True if any achievements were unlocked, otherwise false.
   */
  virtual bool processAchievements() = 0;

  /**
   * @brief Retrieves the game name.
   *
   * The derived game class must override this function, returning the game name.
   *
   * @return A constant reference to a string containing the game name.
   */
  virtual const std::string& getGameName() = 0;

  /**
   * @brief Retrieves a file system path to game data files.
   *
   * The derived game class must override this function, returning the file
   * system path to where game data is stored. This should be a directory where
   * highscores are saved.
   *
   * @return A constant reference to a string containing a file system path.
   */
  virtual const std::string& getGamePath() = 0;

  /**
   * @brief Retrieves a file system path for game log files.
   *
   * The derived game class must override this function, returning the file
   * system path to where game log files are stored.  The games log file is
   * monitored for achievements for example.
   *
   * @return A constant reference to a string containing a file system path.
   */
  virtual const std::string& getLogPath() = 0;

  /**
   * @brief Retrieves a file system path for temporary files.
   *
   * The derived game class must override this function, returning the file
   * system path to a temporary folder.  The games QR code is stored here for
   * example.
   *
   * @return A constant reference to a string containing a file system path.
   */
  virtual const std::string& getTmpPath() = 0;

  /**
   * @brief Retrieves the filename for game audits.
   *
   * The derived game class must override this function, returning the
   * filename containing game audits, typically a json or yaml file.
   *
   * @return A constant reference to a string containing the filename.
   */
  virtual const std::string& getAuditsFile() = 0;

  /**
   * @brief Retrieves the highscores filename.
   *
   * The derived game class must override this function, returning the
   * highscores filename.
   *
   * @return A constant reference to a string containing the filename.
   */
  virtual const std::string& getHighScoresFile() = 0;

  /**
   * @brief Retrieves the filename where last game scores are stored.
   *
   * The derived game class must override this function, returning the
   * last games scores file. Often times it's the same as the high scores file.
   */
  virtual const std::string& getLastScoresFile() = 0;

  /**
   * @brief Retrieves the regex pattern for log filenames.
   *
   * The derived game class must override this function, returning a regex
   * pattern that matches the games log filenames.  This is used to monitor
   * the log directory for new log files to process achievements.
   *
   * @return A constant reference to a regex object.
   */
  virtual const std::regex& getLogFileRegex() = 0;

  /**
   * @brief Sends an i3 command for the specified window.
   *
   * Some games (like TCM) may need to send extra commands if they're
   * using the i3 window manager.
   *
   * @return An integer status code.
   */
  virtual int sendi3cmd() { return 0; }

  /**
   * @brief Sends a sway command for the specified window.
   *
   * Some games (like Evil Dead) use sway to manage Xwayland windows.
   *
   * @return An integer status code.
   */
  virtual int sendswaycmd() { return 0; }
};

using GameFactoryFunction = std::function<std::unique_ptr<GameBase>()>;
extern std::map<std::string, GameFactoryFunction> gameFactories;

#endif
