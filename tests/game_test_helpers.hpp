#ifndef ARKANOID_TESTS_GAME_TEST_HELPERS_HPP
#define ARKANOID_TESTS_GAME_TEST_HELPERS_HPP

#include "arkanoid/core/game.hpp"

#include <algorithm>
#include <cstddef>
#include <gtest/gtest.h>

namespace arkanoid::test {

inline void advanceToCountdownGreen(arkanoid::Game& game) {
    game.update(0.25f);
    game.update(0.35f);
    game.update(0.25f);
    game.update(0.35f);

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
    game.update(1.0f);

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

    game.setInput(false, true, false);
    game.update(4.0f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);

    game.update(2.0f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::LifeLostTransition);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

inline void advanceToLifeLostTransitionAfterBrickHit(arkanoid::Game& game) {
    advanceToBallReady(game);

    game.setInput(true, false, false);
    game.update(1.05f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);

    game.setInput(false, false, true);
    game.update(0.05f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);

    game.setInput(false, false, false);
    game.update(1.30f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    ASSERT_FALSE(game.getState().bricks[0].alive);

    game.setInput(false, true, false);
    game.update(2.50f);
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

    arkanoid::GameState& mutableState = const_cast<arkanoid::GameState&>(game.getState());
    mutableState.ball.x = x;
    mutableState.ball.y = y;
    mutableState.ball.vx = vx;
    mutableState.ball.vy = vy;
}

inline void seedPlayingPaddleAndBallState(arkanoid::Game& game, float paddleX, float ballX, float ballY,
                                          float ballVx, float ballVy) {
    seedPlayingBallState(game, ballX, ballY, ballVx, ballVy);

    arkanoid::GameState& mutableState = const_cast<arkanoid::GameState&>(game.getState());
    mutableState.paddle.x = paddleX;
}

inline std::size_t aliveBrickCount(const arkanoid::GameState& state) {
    return static_cast<std::size_t>(
        std::count_if(state.bricks.begin(), state.bricks.end(),
                      [](const arkanoid::BrickState& brick) { return brick.alive; }));
}

inline void seedFinalBrickHit(arkanoid::Game& game, float ballVy) {
    advanceToPlaying(game);

    arkanoid::GameState& mutableState = const_cast<arkanoid::GameState&>(game.getState());
    for (arkanoid::BrickState& brick : mutableState.bricks) {
        brick.alive = false;
    }

    arkanoid::BrickState& finalBrick = mutableState.bricks[0];
    finalBrick.alive = true;
    mutableState.score = 23u;
    mutableState.ball.x = finalBrick.x + 1.0f;
    mutableState.ball.y = finalBrick.y - 5.0f;
    mutableState.ball.vx = 0.0f;
    mutableState.ball.vy = ballVy;
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
