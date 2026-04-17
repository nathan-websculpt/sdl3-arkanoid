#ifndef ARKANOID_CORE_GAME_HPP
#define ARKANOID_CORE_GAME_HPP

#include "arkanoid/sim/game_state.hpp"

namespace arkanoid
{

class Game
{
  public:
    Game();

    void setInput(bool moveLeftHeld, bool moveRightHeld, bool serveHeld);
    void update(float dt);

    const GameState& getState() const;

  private:
    GameState m_state;
    bool m_moveLeftHeld{};
    bool m_moveRightHeld{};
    bool m_serveHeld{};
    bool m_previousServeHeld{};
};

} // namespace arkanoid

#endif
