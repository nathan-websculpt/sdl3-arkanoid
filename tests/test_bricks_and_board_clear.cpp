#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>

#include "arkanoid/core/game_geometry.hpp"
#include "game_test_helpers.hpp"

using namespace arkanoid::test;

TEST(GameState, BrickHitRemovesExactlyOneBrick) {
    arkanoid::Game game;
    advanceToPlaying(game);

    arkanoid::GameState& mutableState = GameTestAccess::mutableState(game);
    const arkanoid::BrickState firstBrick = mutableState.bricks[0];
    mutableState.ball.x = firstBrick.x + 1.0f;
    mutableState.ball.y = firstBrick.y - 5.0f;
    mutableState.ball.vx = 0.0f;
    mutableState.ball.vy = 100.0f;

    const std::size_t aliveBefore = aliveBrickCount(game.getState());
    const auto scoreBefore = game.getState().score;

    game.setInput(false, false, false);
    game.update(0.10f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_EQ(aliveBrickCount(game.getState()), aliveBefore - static_cast<std::size_t>(1));
    EXPECT_FALSE(game.getState().bricks[0].alive);
    EXPECT_EQ(game.getState().score, scoreBefore + 1u);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, -100.0f);
}

TEST(GameState, RemovedBrickNoLongerCollides) {
    arkanoid::Game game;
    advanceToPlaying(game);

    arkanoid::GameState& mutableState = GameTestAccess::mutableState(game);
    const arkanoid::BrickState firstBrick = mutableState.bricks[0];
    mutableState.ball.x = firstBrick.x + 1.0f;
    mutableState.ball.y = firstBrick.y - 5.0f;
    mutableState.ball.vx = 0.0f;
    mutableState.ball.vy = 100.0f;

    const auto scoreBeforeFirstHit = game.getState().score;

    game.setInput(false, false, false);
    game.update(0.10f);

    ASSERT_FALSE(game.getState().bricks[0].alive);
    ASSERT_EQ(game.getState().score, scoreBeforeFirstHit + 1u);

    mutableState.ball.x = firstBrick.x + 1.0f;
    mutableState.ball.y = firstBrick.y - 5.0f;
    mutableState.ball.vx = 0.0f;
    mutableState.ball.vy = 100.0f;

    const std::size_t aliveBeforeSecondPass = aliveBrickCount(game.getState());
    const auto scoreBeforeSecondPass = game.getState().score;

    game.update(0.10f);

    EXPECT_FALSE(game.getState().bricks[0].alive);
    EXPECT_EQ(aliveBrickCount(game.getState()), aliveBeforeSecondPass);
    EXPECT_EQ(game.getState().score, scoreBeforeSecondPass);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 100.0f);
}

TEST(GameState, NonHitLeavesBricksUnchanged) {
    arkanoid::Game game;
    advanceToPlaying(game);

    arkanoid::GameState& mutableState = GameTestAccess::mutableState(game);
    mutableState.ball.x = 900.0f;
    mutableState.ball.y = 500.0f;
    mutableState.ball.vx = 0.0f;
    mutableState.ball.vy = 50.0f;

    const std::size_t aliveBefore = aliveBrickCount(game.getState());
    const auto scoreBefore = game.getState().score;

    game.setInput(false, false, false);
    game.update(0.05f);

    EXPECT_EQ(aliveBrickCount(game.getState()), aliveBefore);
    EXPECT_EQ(aliveBrickCount(game.getState()), arkanoid::kBrickCount);
    EXPECT_EQ(game.getState().score, scoreBefore);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 50.0f);
}

TEST(GameState, FinalBrickHitTransitionsToBoardClearedTransition) {
    arkanoid::Game game;

    advanceToBoardClearedTransition(game, false);

    EXPECT_EQ(aliveBrickCount(game.getState()), static_cast<std::size_t>(0));
    EXPECT_FALSE(game.getState().bricks[0].alive);
    EXPECT_EQ(game.getState().score, static_cast<std::uint32_t>(arkanoid::kBrickCount));
}

TEST(GameState, BoardClearBeatsBottomLossInSameUpdate) {
    arkanoid::Game game;
    seedFinalBrickHit(game, 1000.0f);

    game.setInput(false, false, false);
    game.update(1.0f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::BoardClearedTransition);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
    EXPECT_GT(game.getState().ball.y, arkanoid::kPlayfieldMaxY);
    EXPECT_EQ(game.getState().score, static_cast<std::uint32_t>(arkanoid::kBrickCount));
}

TEST(GameState, BoardClearRestartRestoresNewGameState) {
    arkanoid::Game canonicalGame;
    const arkanoid::GameState canonicalPreServeState = canonicalGame.getState();

    arkanoid::Game game;
    advanceToBoardClearedTransition(game, false);
    ASSERT_EQ(game.getState().score, static_cast<std::uint32_t>(arkanoid::kBrickCount));
    ASSERT_EQ(aliveBrickCount(game.getState()), static_cast<std::size_t>(0));

    game.update(0.01f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
    EXPECT_EQ(aliveBrickCount(game.getState()), arkanoid::kBrickCount);
    EXPECT_EQ(game.getState().score, 0u);
    EXPECT_FLOAT_EQ(game.getState().paddle.x, canonicalPreServeState.paddle.x);
    EXPECT_FLOAT_EQ(game.getState().ball.x, canonicalPreServeState.ball.x);
    EXPECT_FLOAT_EQ(game.getState().ball.y, canonicalPreServeState.ball.y);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, canonicalPreServeState.ball.vx);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, canonicalPreServeState.ball.vy);
}

TEST(GameState, ServeHeldAcrossBoardClearRestartDoesNotTriggerPlaying) {
    arkanoid::Game game;
    advanceToBoardClearedTransition(game, true);

    game.setInput(false, false, true);
    game.update(0.01f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);

    advanceToCountdownGreen(game);
    game.update(0.10f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::LaunchDrop);

    game.update(1.0f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);

    game.update(0.05f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 0.0f);
}

TEST(GameState, InvalidDtDoesNotMutateBoardClearedTransition) {
    arkanoid::Game game;
    advanceToBoardClearedTransition(game, false);
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
    EXPECT_EQ(aliveBrickCount(game.getState()), aliveBrickCount(stateBeforeInvalidDt));
    EXPECT_EQ(game.getState().score, stateBeforeInvalidDt.score);

    game.update(0.01f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    EXPECT_EQ(aliveBrickCount(game.getState()), arkanoid::kBrickCount);
    EXPECT_EQ(game.getState().score, 0u);
}
