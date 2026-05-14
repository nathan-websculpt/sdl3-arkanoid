#ifndef ARKANOID_TESTS_GAME_TEST_HELPERS_HPP
#define ARKANOID_TESTS_GAME_TEST_HELPERS_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>

#include "arkanoid/core/game.hpp"
#include "arkanoid/core/game_geometry.hpp"

#if !defined(ARKANOID_ENABLE_GAME_TEST_ACCESS)
#error "game_test_helpers.hpp requires ARKANOID_ENABLE_GAME_TEST_ACCESS"
#endif

namespace arkanoid::test {

struct GameTestAccess final {
    static void setBall(arkanoid::Game& game, float x, float y, float vx, float vy) noexcept {
        game.m_state.ball.x = x;
        game.m_state.ball.y = y;
        game.m_state.ball.vx = vx;
        game.m_state.ball.vy = vy;
    }

    static void setPaddleX(arkanoid::Game& game, float paddleX) noexcept {
        game.m_state.paddle.x = paddleX;
    }

    static void prepareFinalBrickHit(arkanoid::Game& game, float ballVy) noexcept {
        for (arkanoid::BrickState& brick : game.m_state.bricks) {
            brick.alive = false;
        }

        arkanoid::BrickState& finalBrick = game.m_state.bricks[0];
        finalBrick.alive = true;
        game.m_state.score = static_cast<std::uint32_t>(arkanoid::kBrickCount - std::size_t{1});
        setBall(game, finalBrick.x + 1.0f, finalBrick.y - 5.0f, 0.0f, ballVy);
    }
};

inline void advanceToCountdownGreen(arkanoid::Game& game) {
    game.update(0.25f);
    game.update(0.25f);
    game.update(0.10f);
    game.update(0.25f);
    game.update(0.25f);
    game.update(0.10f);

    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownGreen);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

inline void advanceToLaunchDrop(arkanoid::Game& game) {
    advanceToCountdownGreen(game);
    game.update(0.10f);

    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::LaunchDrop);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

inline void advanceToBallReady(arkanoid::Game& game) {
    advanceToLaunchDrop(game);
    game.update(0.25f);
    game.update(0.25f);
    game.update(0.25f);

    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

inline void advanceToPlaying(arkanoid::Game& game) {
    advanceToBallReady(game);
    game.setInput(false, false, true);
    game.update(0.05f);

    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

inline void advanceToLifeLostTransition(arkanoid::Game& game) {
    advanceToPlaying(game);

    GameTestAccess::setBall(game, 400.0f, 710.0f, 0.0f, 100.0f);

    game.setInput(false, false, false);
    game.update(0.20f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::LifeLostTransition);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

inline void advanceToLifeLostTransitionAfterBrickHit(arkanoid::Game& game) {
    advanceToPlaying(game);

    const arkanoid::BrickState firstBrick = game.getState().bricks[0];
    GameTestAccess::setBall(game, firstBrick.x + 1.0f, firstBrick.y - 5.0f, 0.0f, 100.0f);
    game.setInput(false, false, false);
    game.update(0.10f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    ASSERT_FALSE(game.getState().bricks[0].alive);

    GameTestAccess::setBall(game, 400.0f, 710.0f, 0.0f, 100.0f);
    game.update(0.20f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::LifeLostTransition);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

inline void advanceLifeLostTransitionToCountdownYellow1(arkanoid::Game& game) {
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::LifeLostTransition);

    game.setInput(false, false, false);
    game.update(0.01f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::ResetTransition);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);

    game.update(0.01f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

inline void seedPlayingBallState(arkanoid::Game& game, float x, float y, float vx, float vy) {
    advanceToPlaying(game);

    GameTestAccess::setBall(game, x, y, vx, vy);
}

inline void seedPlayingPaddleAndBallState(arkanoid::Game& game, float paddleX, float ballX,
                                          float ballY, float ballVx, float ballVy) {
    seedPlayingBallState(game, ballX, ballY, ballVx, ballVy);

    GameTestAccess::setPaddleX(game, paddleX);
}

inline std::size_t aliveBrickCount(const arkanoid::GameState& state) {
    return static_cast<std::size_t>(
        std::count_if(state.bricks.begin(), state.bricks.end(),
                      [](const arkanoid::BrickState& brick) { return brick.alive; }));
}

inline void setBallReadyPaddleX(arkanoid::Game& game, float paddleX) {
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);
    GameTestAccess::setPaddleX(game, paddleX);
}

inline void setBallForFirstBrickHit(arkanoid::Game& game, float ballVy) {
    const arkanoid::BrickState firstBrick = game.getState().bricks[0];
    GameTestAccess::setBall(game, firstBrick.x + 1.0f, firstBrick.y - 5.0f, 0.0f, ballVy);
}

inline void setBallForNonHit(arkanoid::Game& game) {
    GameTestAccess::setBall(game, 900.0f, 500.0f, 0.0f, 50.0f);
}

inline void seedFinalBrickHit(arkanoid::Game& game, float ballVy) {
    advanceToPlaying(game);
    GameTestAccess::prepareFinalBrickHit(game, ballVy);
}

inline void advanceToBoardClearedTransition(arkanoid::Game& game, bool serveHeld) {
    seedFinalBrickHit(game, 100.0f);

    game.setInput(false, false, serveHeld);
    game.update(0.10f);

    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::BoardClearedTransition);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

} // namespace arkanoid::test

#endif
