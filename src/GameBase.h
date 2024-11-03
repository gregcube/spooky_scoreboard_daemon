#ifndef _GAME_BASE_H
#define _GAME_BASE_H

#include <string>
#include <memory>
#include <functional>

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
   * @brief Sends an i3 command for the specified window.
   *
   * Some games (like TCM) may need to send extra commands if they're
   * using the i3 window manager.
   *
   * @param win A reference to an X11 window for which the i3 commands are sent.
   * @return An integer status code.
   */
  virtual int sendi3cmd(const Window& win) { return 0; }
};

using GameFactoryFunction = std::function<std::unique_ptr<GameBase>()>;
extern std::map<std::string, GameFactoryFunction> gameFactories;

#endif
