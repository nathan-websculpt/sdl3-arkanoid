#include <gtest/gtest.h>

#include "arkanoid/core/game_geometry.hpp"
#include "game_test_helpers.hpp"

using namespace arkanoid::test;

TEST(GameState, StartsInCountdown) {
    arkanoid::Game game;

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().paddle.x, arkanoid::kCanonicalPreServeCenterX);
    EXPECT_FLOAT_EQ(game.getState().ball.x, arkanoid::kCanonicalPreServeCenterX);
    EXPECT_FLOAT_EQ(game.getState().ball.y, arkanoid::kCanonicalPreServeBallY);
}

TEST(GameState, AccumulatesTimerWithoutTransition) {
    arkanoid::Game game;

    game.update(0.10f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.10f);
}

TEST(GameState, TransitionsFromCountdownYellow1ToCountdownPause1AtBoundary) {
    arkanoid::Game game;

    game.update(0.25f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownPause1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

TEST(GameState, TransitionsFromCountdownPause1ToCountdownYellow2AtBoundary) {
    arkanoid::Game game;

    game.update(0.25f);
    game.update(0.25f);
    game.update(0.10f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow2);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

TEST(GameState, TransitionsFromCountdownYellow2ToCountdownPause2AtBoundary) {
    arkanoid::Game game;

    game.update(0.25f);
    game.update(0.25f);
    game.update(0.10f);
    game.update(0.25f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownPause2);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

TEST(GameState, TransitionsFromCountdownPause2ToCountdownGreenAtBoundary) {
    arkanoid::Game game;

    game.update(0.25f);
    game.update(0.25f);
    game.update(0.10f);
    game.update(0.25f);
    game.update(0.25f);
    game.update(0.10f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownGreen);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

TEST(GameState, TransitionsFromCountdownGreenToLaunchDropAtBoundary) {
    arkanoid::Game game;

    advanceToCountdownGreen(game);
    game.setInput(false, true, false);
    game.update(0.05f);
    game.setInput(false, false, false);
    game.update(0.05f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::LaunchDrop);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().paddle.x, 500.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, 500.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.y, arkanoid::kCanonicalPreServeBallY);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 400.0f);
}

TEST(GameState, LaunchDropFollowsPaddleAndIntegratesYBeforeServe) {
    arkanoid::Game game;

    advanceToLaunchDrop(game);

    const float launchDropY = game.getState().ball.y;

    game.setInput(false, true, false);
    game.update(0.05f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::LaunchDrop);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.05f);
    EXPECT_FLOAT_EQ(game.getState().paddle.x, 500.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, game.getState().paddle.x);
    EXPECT_FLOAT_EQ(game.getState().ball.y, launchDropY + (400.0f * 0.05f));
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 400.0f);
}

TEST(GameState, LaunchDropClampsAndTransitionsToBallReady) {
    arkanoid::Game firstGame;
    advanceToLaunchDrop(firstGame);

    arkanoid::Game secondGame;
    advanceToLaunchDrop(secondGame);

    firstGame.update(0.25f);
    firstGame.update(0.25f);
    firstGame.update(0.25f);

    EXPECT_EQ(firstGame.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(firstGame.getState().phaseTime, 0.0f);
    EXPECT_FLOAT_EQ(firstGame.getState().ball.x, firstGame.getState().paddle.x);
    const float contactY = firstGame.getState().ball.y;
    EXPECT_FLOAT_EQ(firstGame.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(firstGame.getState().ball.vy, 0.0f);

    secondGame.update(0.25f);
    secondGame.update(0.25f);
    secondGame.update(0.25f);

    EXPECT_EQ(secondGame.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(secondGame.getState().phaseTime, 0.0f);
    EXPECT_FLOAT_EQ(secondGame.getState().ball.x, secondGame.getState().paddle.x);
    EXPECT_FLOAT_EQ(secondGame.getState().ball.y, contactY);
    EXPECT_FLOAT_EQ(secondGame.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(secondGame.getState().ball.vy, 0.0f);
}

TEST(GameState, PaddleMovementIsAppliedWhileBallReady) {
    arkanoid::Game game;
    advanceToBallReady(game);

    game.setInput(false, true, false);
    game.update(0.25f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(game.getState().paddle.x, 580.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, game.getState().paddle.x);
    EXPECT_FLOAT_EQ(game.getState().ball.y, arkanoid::kPaddleTopY);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 0.0f);
}

TEST(GameState, PaddleMovementClampsToBoundsWhileBallReady) {
    arkanoid::Game game;
    advanceToBallReady(game);

    game.setInput(true, false, false);
    setBallReadyPaddleX(game, arkanoid::kPlayfieldMinX + 10.0f);
    game.update(0.25f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(game.getState().paddle.x, arkanoid::kPlayfieldMinX);
    EXPECT_FLOAT_EQ(game.getState().ball.x, arkanoid::kPlayfieldMinX);

    game.setInput(false, true, false);
    setBallReadyPaddleX(game, arkanoid::kPlayfieldMaxX - 10.0f);
    game.update(0.25f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(game.getState().paddle.x, arkanoid::kPlayfieldMaxX);
    EXPECT_FLOAT_EQ(game.getState().ball.x, arkanoid::kPlayfieldMaxX);
}

TEST(GameState, OpposingHorizontalInputProducesNoMovementWhileBallReady) {
    arkanoid::Game game;
    advanceToBallReady(game);

    game.setInput(false, true, false);
    game.update(0.25f);
    const float xAfterRightMove = game.getState().paddle.x;

    game.setInput(true, true, false);
    game.update(0.25f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(game.getState().paddle.x, xAfterRightMove);
    EXPECT_FLOAT_EQ(game.getState().ball.x, xAfterRightMove);
    EXPECT_FLOAT_EQ(game.getState().ball.y, arkanoid::kPaddleTopY);
}

TEST(GameState, ServeHeldAcrossBallReadyEntryDoesNotTriggerPlaying) {
    arkanoid::Game game;
    advanceToLaunchDrop(game);

    game.setInput(false, false, true);
    game.update(0.25f);
    game.update(0.25f);
    game.update(0.25f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 0.0f);

    game.update(0.05f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 0.0f);
}

TEST(GameState, ServeRisingEdgeTransitionsFromBallReadyToPlayingWithServeVelocity) {
    arkanoid::Game game;
    advanceToBallReady(game);

    game.setInput(false, false, true);
    game.update(0.05f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, game.getState().paddle.x);
    EXPECT_FLOAT_EQ(game.getState().ball.y, arkanoid::kPaddleTopY);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, -400.0f);
}

TEST(GameState, FirstPlayingUpdateAfterServeIntegratesFreeMotionOnly) {
    arkanoid::Game game;
    advanceToBallReady(game);

    game.setInput(false, false, true);
    game.update(0.05f);

    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
    ASSERT_FLOAT_EQ(game.getState().ball.x, game.getState().paddle.x);
    ASSERT_FLOAT_EQ(game.getState().ball.y, arkanoid::kPaddleTopY);
    ASSERT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    ASSERT_FLOAT_EQ(game.getState().ball.vy, -400.0f);

    const float startingBallX = game.getState().ball.x;
    const float startingBallY = game.getState().ball.y;

    game.setInput(false, false, false);
    game.update(0.05f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.05f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, startingBallX);
    EXPECT_FLOAT_EQ(game.getState().ball.y, startingBallY + (-400.0f * 0.05f));
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, -400.0f);
}

TEST(GameState, PlayingIntegratesDeterministicFreeMotionAcrossMultipleSteps) {
    arkanoid::Game game;
    advanceToPlaying(game);

    const float startingBallX = game.getState().ball.x;
    const float startingBallY = game.getState().ball.y;

    game.setInput(false, false, false);
    game.update(0.02f);
    game.update(0.03f);
    game.update(0.05f);

    const float totalDt = 0.02f + 0.03f + 0.05f;

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, totalDt);
    EXPECT_FLOAT_EQ(game.getState().ball.x, startingBallX);
    EXPECT_FLOAT_EQ(game.getState().ball.y, startingBallY + (-400.0f * totalDt));
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, -400.0f);
}
