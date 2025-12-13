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
public:
  virtual ~GameBase() = default;

  static std::unique_ptr<GameBase> create(const std::string& gameName);

  void setUrl(WebSocket* ws);
  std::string getUrl() const { return !gameUrl.empty() ? gameUrl : ""; }

  enum class ScoreType { High, Last, Mode };
  void uploadScores(const Json::Value& scores, ScoreType type, WebSocket* ws);

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

private:
  std::string gameUrl;
};

using GameFactoryFunction = std::function<std::unique_ptr<GameBase>()>;
extern std::map<std::string, GameFactoryFunction> gameFactories;

// vim: set ts=2 sw=2 expandtab:

