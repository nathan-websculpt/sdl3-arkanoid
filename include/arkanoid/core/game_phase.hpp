#ifndef ARKANOID_CORE_GAME_PHASE_HPP
#define ARKANOID_CORE_GAME_PHASE_HPP

namespace arkanoid
{

enum class GamePhase
{
    CountdownYellow1,
    CountdownPause1,
    CountdownYellow2,
    CountdownPause2,
    CountdownGreen,
    LaunchDrop,
    BallReady,
    Playing,
    LifeLostTransition,
    ResetTransition
};

} // namespace arkanoid

#endif
