#include <gtest/gtest.h>
#include <limits>

#include "arkanoid/core/game_geometry.hpp"
#include "game_test_helpers.hpp"

using namespace arkanoid::test;

TEST(GameState, LeftWallBounceReflectsAndClamps) {
    arkanoid::Game game;
    seedPlayingBallState(game, 10.0f, 300.0f, -100.0f, 0.0f);
    game.setInput(false, false, false);

    game.update(0.20f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.20f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, arkanoid::kPlayfieldMinX);
    EXPECT_FLOAT_EQ(game.getState().ball.y, 300.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 100.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 0.0f);
}

TEST(GameState, RightWallBounceReflectsAndClamps) {
    arkanoid::Game game;
    seedPlayingBallState(game, 950.0f, 300.0f, 100.0f, 0.0f);
    game.setInput(false, false, false);

    game.update(0.20f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.20f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, arkanoid::kPlayfieldMaxX);
    EXPECT_FLOAT_EQ(game.getState().ball.y, 300.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, -100.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 0.0f);
}

TEST(GameState, TopWallBounceReflectsAndClamps) {
    arkanoid::Game game;
    seedPlayingBallState(game, 400.0f, 10.0f, 0.0f, -100.0f);
    game.setInput(false, false, false);

    game.update(0.20f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.20f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, 400.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.y, arkanoid::kPlayfieldMinY);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 100.0f);
}

TEST(GameState, DirectDownwardPaddleHitReflectsUpward) {
    arkanoid::Game game;
    seedPlayingPaddleAndBallState(game, 400.0f, 400.0f, 610.0f, 0.0f, 100.0f);
    game.setInput(false, false, false);

    game.update(0.20f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.20f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, 400.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.y, arkanoid::kPaddleTopY);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, -100.0f);
}

TEST(GameState, SymmetricPaddleHitsProduceOppositeHorizontalDirections) {
    arkanoid::Game leftHitGame;
    seedPlayingPaddleAndBallState(leftHitGame, 400.0f, 380.0f, 610.0f, 0.0f, 100.0f);
    leftHitGame.setInput(false, false, false);

    arkanoid::Game rightHitGame;
    seedPlayingPaddleAndBallState(rightHitGame, 400.0f, 420.0f, 610.0f, 0.0f, 100.0f);
    rightHitGame.setInput(false, false, false);

    leftHitGame.update(0.20f);
    rightHitGame.update(0.20f);

    EXPECT_EQ(leftHitGame.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_EQ(rightHitGame.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_LT(leftHitGame.getState().ball.vx, 0.0f);
    EXPECT_GT(rightHitGame.getState().ball.vx, 0.0f);
    EXPECT_LT(leftHitGame.getState().ball.vy, 0.0f);
    EXPECT_LT(rightHitGame.getState().ball.vy, 0.0f);
}

TEST(GameState, PaddleHitHorizontalDeflectionMagnitudeIncreasesWithOffset) {
    arkanoid::Game nearCenterHitGame;
    seedPlayingPaddleAndBallState(nearCenterHitGame, 400.0f, 410.0f, 610.0f, 0.0f, 100.0f);
    nearCenterHitGame.setInput(false, false, false);

    arkanoid::Game farFromCenterHitGame;
    seedPlayingPaddleAndBallState(farFromCenterHitGame, 400.0f, 460.0f, 610.0f, 0.0f, 100.0f);
    farFromCenterHitGame.setInput(false, false, false);

    nearCenterHitGame.update(0.20f);
    farFromCenterHitGame.update(0.20f);

    EXPECT_EQ(nearCenterHitGame.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_EQ(farFromCenterHitGame.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_GT(nearCenterHitGame.getState().ball.vx, 0.0f);
    EXPECT_GT(farFromCenterHitGame.getState().ball.vx, 0.0f);
    EXPECT_GT(farFromCenterHitGame.getState().ball.vx, nearCenterHitGame.getState().ball.vx);
    EXPECT_LT(nearCenterHitGame.getState().ball.vy, 0.0f);
    EXPECT_LT(farFromCenterHitGame.getState().ball.vy, 0.0f);
}

TEST(GameState, PaddleMissDoesNotBounce) {
    arkanoid::Game game;
    seedPlayingPaddleAndBallState(game, 400.0f, 500.0f, 610.0f, 0.0f, 100.0f);
    game.setInput(false, false, false);

    game.update(0.20f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.20f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, 500.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.y, 630.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 100.0f);
}

TEST(GameState, WallThenPaddleOrderingIsDeterministic) {
    arkanoid::Game game;
    seedPlayingPaddleAndBallState(game, 20.0f, 20.0f, 610.0f, -600.0f, 100.0f);
    game.setInput(false, false, false);

    game.update(0.20f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.20f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, arkanoid::kPlayfieldMinX);
    EXPECT_FLOAT_EQ(game.getState().ball.y, arkanoid::kPaddleTopY);
    EXPECT_LT(game.getState().ball.vx, 0.0f);
    EXPECT_LT(game.getState().ball.vy, 0.0f);
}

TEST(GameState, InvalidDtDoesNotMutatePendingPaddleCollision) {
    arkanoid::Game game;
    seedPlayingPaddleAndBallState(game, 400.0f, 400.0f, 610.0f, 0.0f, 100.0f);
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

    game.update(0.20f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, stateBeforeInvalidDt.phaseTime + 0.20f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, 400.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.y, arkanoid::kPaddleTopY);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, -100.0f);
}
