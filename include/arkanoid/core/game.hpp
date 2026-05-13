#ifndef ARKANOID_CORE_GAME_HPP
#define ARKANOID_CORE_GAME_HPP

#include "arkanoid/sim/game_state.hpp"

namespace arkanoid {

namespace test {
struct GameTestAccess;
} // namespace test

class Game final {
  public:
    Game();

    // Game owns one authoritative simulation state; copying or moving would obscure that identity
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;
    Game(Game&&) = delete;
    Game& operator=(Game&&) = delete;
    ~Game() = default;

    void setInput(bool moveLeftHeld, bool moveRightHeld, bool serveHeld);
    void update(float dt);

    [[nodiscard]] const GameState& getState() const;

  private:
    friend struct test::GameTestAccess;

    GameState m_state;
    bool m_moveLeftHeld{};
    bool m_moveRightHeld{};
    bool m_serveHeld{};
    bool m_previousServeHeld{};
};

} // namespace arkanoid

#endif
