#include "game_test_helpers.hpp"

#include <gtest/gtest.h>

#include <cstddef>

using namespace arkanoid::test;

TEST(GameState, CrossingBottomBoundaryTransitionsToLifeLostTransition) {
    arkanoid::Game crossingGame;
    seedPlayingBallState(crossingGame, 400.0f, 710.0f, 0.0f, 100.0f);
    crossingGame.setInput(false, false, false);

    crossingGame.update(0.20f);

    EXPECT_EQ(crossingGame.getState().phase, arkanoid::GamePhase::LifeLostTransition);
    EXPECT_FLOAT_EQ(crossingGame.getState().phaseTime, 0.0f);
    EXPECT_FLOAT_EQ(crossingGame.getState().ball.x, 400.0f);
    EXPECT_FLOAT_EQ(crossingGame.getState().ball.y, 730.0f);
    EXPECT_FLOAT_EQ(crossingGame.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(crossingGame.getState().ball.vy, 100.0f);

    arkanoid::Game equalityGame;
    seedPlayingBallState(equalityGame, 400.0f, 700.0f, 0.0f, 100.0f);
    equalityGame.setInput(false, false, false);

    equalityGame.update(0.20f);

    EXPECT_EQ(equalityGame.getState().phase, arkanoid::GamePhase::LifeLostTransition);
    EXPECT_FLOAT_EQ(equalityGame.getState().phaseTime, 0.0f);
    EXPECT_FLOAT_EQ(equalityGame.getState().ball.y, 720.0f);
}

TEST(GameState, NonCrossingBottomBoundaryRemainsPlaying) {
    arkanoid::Game game;
    seedPlayingBallState(game, 400.0f, 690.0f, 0.0f, 100.0f);
    game.setInput(false, false, false);

    game.update(0.20f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.20f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, 400.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.y, 710.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 100.0f);
}

TEST(GameState, BottomTransitionIsSimulationOnly) {
    arkanoid::Game game;
    seedPlayingBallState(game, 400.0f, 710.0f, 0.0f, 100.0f);

    game.update(0.20f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::LifeLostTransition);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

TEST(GameState, BottomLossEventuallyResetsIntoCountdownYellow1) {
    arkanoid::Game game;
    advanceToLifeLostTransition(game);

    game.setInput(false, false, false);
    game.update(0.01f);
    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::ResetTransition);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);

    game.update(0.01f);
    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

TEST(GameState, ScoreIsPreservedAcrossLifeLossReset) {
    arkanoid::Game game;
    advanceToLifeLostTransitionAfterBrickHit(game);

    const auto scoreBeforeReset = game.getState().score;
    ASSERT_GT(scoreBeforeReset, 0u);

    advanceLifeLostTransitionToCountdownYellow1(game);

    EXPECT_EQ(game.getState().score, scoreBeforeReset);
}

TEST(GameState, RemovedBricksRemainRemovedAcrossLifeLossReset) {
    arkanoid::Game game;
    advanceToLifeLostTransitionAfterBrickHit(game);

    const std::size_t aliveBeforeReset = aliveBrickCount(game.getState());
    ASSERT_FALSE(game.getState().bricks[0].alive);

    advanceLifeLostTransitionToCountdownYellow1(game);

    EXPECT_FALSE(game.getState().bricks[0].alive);
    EXPECT_EQ(aliveBrickCount(game.getState()), aliveBeforeReset);
}

TEST(GameState, PaddleAndBallRestoreToCanonicalPreServePoseOnReset) {
    arkanoid::Game canonicalGame;
    const arkanoid::GameState canonicalPreServeState = canonicalGame.getState();

    arkanoid::Game game;
    advanceToLifeLostTransition(game);
    advanceLifeLostTransitionToCountdownYellow1(game);

    EXPECT_FLOAT_EQ(game.getState().paddle.x, canonicalPreServeState.paddle.x);
    EXPECT_FLOAT_EQ(game.getState().ball.x, canonicalPreServeState.ball.x);
    EXPECT_FLOAT_EQ(game.getState().ball.y, canonicalPreServeState.ball.y);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, canonicalPreServeState.ball.vx);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, canonicalPreServeState.ball.vy);
}

