#include "game_test_helpers.hpp"

#include <gtest/gtest.h>

#include <limits>

using namespace arkanoid::test;

TEST(GameState, OvershootResetsTimerWithoutCarry) {
    arkanoid::Game game;

    game.update(0.30f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownPause1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

TEST(GameState, IgnoresNonFiniteDt) {
    arkanoid::Game game;

    game.update(0.10f);
    game.update(std::numeric_limits<float>::quiet_NaN());
    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.10f);

    game.update(std::numeric_limits<float>::infinity());
    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.10f);
}

TEST(GameState, IgnoresNonPositiveDt) {
    arkanoid::Game game;

    game.update(0.10f);
    game.update(0.0f);
    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.10f);

    game.update(-0.25f);
    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.10f);
}

TEST(GameState, InvalidDtDoesNotMutateBallReadyStateOrConsumeServeEdge) {
    arkanoid::Game game;
    advanceToBallReady(game);
    game.setInput(false, false, true);

    const arkanoid::GameState stateBeforeInvalidDt = game.getState();

    game.update(std::numeric_limits<float>::quiet_NaN());
    game.update(std::numeric_limits<float>::infinity());
    game.update(0.0f);
    game.update(-0.25f);

    EXPECT_EQ(game.getState().phase, stateBeforeInvalidDt.phase);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, stateBeforeInvalidDt.phaseTime);
    EXPECT_FLOAT_EQ(game.getState().paddle.x, stateBeforeInvalidDt.paddle.x);
    EXPECT_FLOAT_EQ(game.getState().ball.x, stateBeforeInvalidDt.ball.x);
    EXPECT_FLOAT_EQ(game.getState().ball.y, stateBeforeInvalidDt.ball.y);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, stateBeforeInvalidDt.ball.vx);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, stateBeforeInvalidDt.ball.vy);

    game.update(0.05f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, -400.0f);
}

TEST(GameState, InvalidDtDoesNotMutatePlayingState) {
    arkanoid::Game game;
    advanceToPlaying(game);
    game.setInput(false, false, false);

    const arkanoid::GameState stateBeforeInvalidDt = game.getState();

    game.update(std::numeric_limits<float>::quiet_NaN());
    game.update(std::numeric_limits<float>::infinity());
    game.update(0.0f);
    game.update(-0.25f);

    EXPECT_EQ(game.getState().phase, stateBeforeInvalidDt.phase);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, stateBeforeInvalidDt.phaseTime);
    EXPECT_FLOAT_EQ(game.getState().paddle.x, stateBeforeInvalidDt.paddle.x);
    EXPECT_FLOAT_EQ(game.getState().ball.x, stateBeforeInvalidDt.ball.x);
    EXPECT_FLOAT_EQ(game.getState().ball.y, stateBeforeInvalidDt.ball.y);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, stateBeforeInvalidDt.ball.vx);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, stateBeforeInvalidDt.ball.vy);

    game.update(0.05f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, stateBeforeInvalidDt.phaseTime + 0.05f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, stateBeforeInvalidDt.ball.x);
    EXPECT_FLOAT_EQ(game.getState().ball.y,
                    stateBeforeInvalidDt.ball.y + (stateBeforeInvalidDt.ball.vy * 0.05f));
    EXPECT_FLOAT_EQ(game.getState().ball.vx, stateBeforeInvalidDt.ball.vx);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, stateBeforeInvalidDt.ball.vy);
}

