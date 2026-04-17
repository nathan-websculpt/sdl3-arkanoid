#ifndef ARKANOID_SIM_GAME_STATE_HPP
#define ARKANOID_SIM_GAME_STATE_HPP

#include "arkanoid/core/game_phase.hpp"

#include <array>
#include <cstdint>

namespace arkanoid
{

struct PaddleState
{
    float x{};
};

struct BallState
{
    float x{};
    float y{};
    float vx{};
    float vy{};
};

struct BrickState
{
    float x{};
    float y{};
    bool alive{};
};

struct GameState
{
    PaddleState paddle;
    BallState ball;
    std::array<BrickState, 24> bricks{};
    std::uint32_t score{};

    GamePhase phase{};
    float phaseTime{};
};

} // namespace arkanoid

#endif
