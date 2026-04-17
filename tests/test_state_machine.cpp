#include "arkanoid/core/game.hpp"

#include <algorithm>
#include <cstddef>
#include <gtest/gtest.h>
#include <limits>

namespace
{

void advanceToCountdownGreen(arkanoid::Game& game)
{
    game.update(0.25f);
    game.update(0.35f);
    game.update(0.25f);
    game.update(0.35f);

    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownGreen);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

void advanceToLaunchDrop(arkanoid::Game& game)
{
    advanceToCountdownGreen(game);
    game.update(0.10f);

    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::LaunchDrop);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

void advanceToBallReady(arkanoid::Game& game)
{
    advanceToLaunchDrop(game);
    game.update(1.0f);

    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

void advanceToPlaying(arkanoid::Game& game)
{
    advanceToBallReady(game);
    game.setInput(false, false, true);
    game.update(0.05f);

    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

void advanceToLifeLostTransition(arkanoid::Game& game)
{
    advanceToPlaying(game);

    game.setInput(false, true, false);
    game.update(4.0f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);

    game.update(2.0f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::LifeLostTransition);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

void advanceToLifeLostTransitionAfterBrickHit(arkanoid::Game& game)
{
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

void advanceLifeLostTransitionToCountdownYellow1(arkanoid::Game& game)
{
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::LifeLostTransition);

    game.setInput(false, false, false);
    game.update(0.01f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::ResetTransition);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);

    game.update(0.01f);
    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

void seedPlayingBallState(arkanoid::Game& game, float x, float y, float vx, float vy)
{
    advanceToPlaying(game);

    arkanoid::GameState& mutableState = const_cast<arkanoid::GameState&>(game.getState());
    mutableState.ball.x = x;
    mutableState.ball.y = y;
    mutableState.ball.vx = vx;
    mutableState.ball.vy = vy;
}

void seedPlayingPaddleAndBallState(arkanoid::Game& game, float paddleX, float ballX, float ballY,
                                   float ballVx, float ballVy)
{
    seedPlayingBallState(game, ballX, ballY, ballVx, ballVy);

    arkanoid::GameState& mutableState = const_cast<arkanoid::GameState&>(game.getState());
    mutableState.paddle.x = paddleX;
}

std::size_t aliveBrickCount(const arkanoid::GameState& state)
{
    return static_cast<std::size_t>(std::count_if(state.bricks.begin(), state.bricks.end(),
                                                  [](const arkanoid::BrickState& brick)
                                                  { return brick.alive; }));
}

} // namespace

TEST(GameState, StartsInCountdown)
{
    arkanoid::Game game;

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().paddle.x, 480.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, 480.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.y, 368.0f);
}

TEST(GameState, AccumulatesTimerWithoutTransition)
{
    arkanoid::Game game;

    game.update(0.10f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.10f);
}

TEST(GameState, TransitionsFromCountdownYellow1ToCountdownPause1AtBoundary)
{
    arkanoid::Game game;

    game.update(0.25f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownPause1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

TEST(GameState, TransitionsFromCountdownPause1ToCountdownYellow2AtBoundary)
{
    arkanoid::Game game;

    game.update(0.25f);
    game.update(0.35f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow2);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

TEST(GameState, TransitionsFromCountdownYellow2ToCountdownPause2AtBoundary)
{
    arkanoid::Game game;

    game.update(0.25f);
    game.update(0.35f);
    game.update(0.25f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownPause2);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

TEST(GameState, TransitionsFromCountdownPause2ToCountdownGreenAtBoundary)
{
    arkanoid::Game game;

    game.update(0.25f);
    game.update(0.35f);
    game.update(0.25f);
    game.update(0.35f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownGreen);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

TEST(GameState, TransitionsFromCountdownGreenToLaunchDropAtBoundary)
{
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
    EXPECT_FLOAT_EQ(game.getState().ball.y, 368.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 400.0f);
}

TEST(GameState, LaunchDropFollowsPaddleAndIntegratesYBeforeServe)
{
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

TEST(GameState, LaunchDropClampsAndTransitionsToBallReady)
{
    arkanoid::Game firstGame;
    advanceToLaunchDrop(firstGame);

    arkanoid::Game secondGame;
    advanceToLaunchDrop(secondGame);

    firstGame.update(10.0f);

    EXPECT_EQ(firstGame.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(firstGame.getState().phaseTime, 0.0f);
    EXPECT_FLOAT_EQ(firstGame.getState().ball.x, firstGame.getState().paddle.x);
    const float contactY = firstGame.getState().ball.y;
    EXPECT_FLOAT_EQ(firstGame.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(firstGame.getState().ball.vy, 0.0f);

    secondGame.update(20.0f);

    EXPECT_EQ(secondGame.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(secondGame.getState().phaseTime, 0.0f);
    EXPECT_FLOAT_EQ(secondGame.getState().ball.x, secondGame.getState().paddle.x);
    EXPECT_FLOAT_EQ(secondGame.getState().ball.y, contactY);
    EXPECT_FLOAT_EQ(secondGame.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(secondGame.getState().ball.vy, 0.0f);
}

TEST(GameState, PaddleMovementIsAppliedWhileBallReady)
{
    arkanoid::Game game;
    advanceToBallReady(game);

    game.setInput(false, true, false);
    game.update(0.25f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(game.getState().paddle.x, 580.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, game.getState().paddle.x);
    EXPECT_FLOAT_EQ(game.getState().ball.y, 620.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 0.0f);
}

TEST(GameState, PaddleMovementClampsToBoundsWhileBallReady)
{
    arkanoid::Game game;
    advanceToBallReady(game);

    game.setInput(true, false, false);
    game.update(10.0f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(game.getState().paddle.x, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, 0.0f);

    game.setInput(false, true, false);
    game.update(10.0f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(game.getState().paddle.x, 960.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, 960.0f);
}

TEST(GameState, OpposingHorizontalInputProducesNoMovementWhileBallReady)
{
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
    EXPECT_FLOAT_EQ(game.getState().ball.y, 620.0f);
}

TEST(GameState, ServeHeldAcrossBallReadyEntryDoesNotTriggerPlaying)
{
    arkanoid::Game game;
    advanceToLaunchDrop(game);

    game.setInput(false, false, true);
    game.update(1.0f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 0.0f);

    game.update(0.05f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::BallReady);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 0.0f);
}

TEST(GameState, ServeRisingEdgeTransitionsFromBallReadyToPlayingWithServeVelocity)
{
    arkanoid::Game game;
    advanceToBallReady(game);

    game.setInput(false, false, true);
    game.update(0.05f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, game.getState().paddle.x);
    EXPECT_FLOAT_EQ(game.getState().ball.y, 620.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, -400.0f);
}

TEST(GameState, FirstPlayingUpdateAfterServeIntegratesFreeMotionOnly)
{
    arkanoid::Game game;
    advanceToBallReady(game);

    game.setInput(false, false, true);
    game.update(0.05f);

    ASSERT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    ASSERT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
    ASSERT_FLOAT_EQ(game.getState().ball.x, game.getState().paddle.x);
    ASSERT_FLOAT_EQ(game.getState().ball.y, 620.0f);
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

TEST(GameState, PlayingIntegratesDeterministicFreeMotionAcrossMultipleSteps)
{
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

TEST(GameState, OvershootResetsTimerWithoutCarry)
{
    arkanoid::Game game;

    game.update(0.30f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownPause1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

TEST(GameState, IgnoresNonFiniteDt)
{
    arkanoid::Game game;

    game.update(0.10f);
    game.update(std::numeric_limits<float>::quiet_NaN());
    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.10f);

    game.update(std::numeric_limits<float>::infinity());
    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.10f);
}

TEST(GameState, IgnoresNonPositiveDt)
{
    arkanoid::Game game;

    game.update(0.10f);
    game.update(0.0f);
    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.10f);

    game.update(-0.25f);
    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::CountdownYellow1);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.10f);
}

TEST(GameState, InvalidDtDoesNotMutateBallReadyStateOrConsumeServeEdge)
{
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

TEST(GameState, InvalidDtDoesNotMutatePlayingState)
{
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

TEST(GameState, LeftWallBounceReflectsAndClamps)
{
    arkanoid::Game game;
    seedPlayingBallState(game, 10.0f, 300.0f, -100.0f, 0.0f);
    game.setInput(false, false, false);

    game.update(0.20f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.20f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.y, 300.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 100.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 0.0f);
}

TEST(GameState, RightWallBounceReflectsAndClamps)
{
    arkanoid::Game game;
    seedPlayingBallState(game, 950.0f, 300.0f, 100.0f, 0.0f);
    game.setInput(false, false, false);

    game.update(0.20f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.20f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, 960.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.y, 300.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, -100.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 0.0f);
}

TEST(GameState, TopWallBounceReflectsAndClamps)
{
    arkanoid::Game game;
    seedPlayingBallState(game, 400.0f, 10.0f, 0.0f, -100.0f);
    game.setInput(false, false, false);

    game.update(0.20f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.20f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, 400.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.y, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 100.0f);
}

TEST(GameState, DirectDownwardPaddleHitReflectsUpward)
{
    arkanoid::Game game;
    seedPlayingPaddleAndBallState(game, 400.0f, 400.0f, 610.0f, 0.0f, 100.0f);
    game.setInput(false, false, false);

    game.update(0.20f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.20f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, 400.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.y, 620.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, -100.0f);
}

TEST(GameState, SymmetricPaddleHitsProduceOppositeHorizontalDirections)
{
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

TEST(GameState, PaddleHitHorizontalDeflectionMagnitudeIncreasesWithOffset)
{
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

TEST(GameState, PaddleMissDoesNotBounce)
{
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

TEST(GameState, WallThenPaddleOrderingIsDeterministic)
{
    arkanoid::Game game;
    seedPlayingPaddleAndBallState(game, 20.0f, 20.0f, 610.0f, -600.0f, 100.0f);
    game.setInput(false, false, false);

    game.update(0.20f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::Playing);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.20f);
    EXPECT_FLOAT_EQ(game.getState().ball.x, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.y, 620.0f);
    EXPECT_LT(game.getState().ball.vx, 0.0f);
    EXPECT_LT(game.getState().ball.vy, 0.0f);
}

TEST(GameState, InvalidDtDoesNotMutatePendingPaddleCollision)
{
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
    EXPECT_FLOAT_EQ(game.getState().ball.y, 620.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vx, 0.0f);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, -100.0f);
}

TEST(GameState, BrickHitRemovesExactlyOneBrick)
{
    arkanoid::Game game;
    advanceToPlaying(game);

    arkanoid::GameState& mutableState = const_cast<arkanoid::GameState&>(game.getState());
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

TEST(GameState, RemovedBrickNoLongerCollides)
{
    arkanoid::Game game;
    advanceToPlaying(game);

    arkanoid::GameState& mutableState = const_cast<arkanoid::GameState&>(game.getState());
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

TEST(GameState, NonHitLeavesBricksUnchanged)
{
    arkanoid::Game game;
    advanceToPlaying(game);

    arkanoid::GameState& mutableState = const_cast<arkanoid::GameState&>(game.getState());
    mutableState.ball.x = 900.0f;
    mutableState.ball.y = 500.0f;
    mutableState.ball.vx = 0.0f;
    mutableState.ball.vy = 50.0f;

    const std::size_t aliveBefore = aliveBrickCount(game.getState());
    const auto scoreBefore = game.getState().score;

    game.setInput(false, false, false);
    game.update(0.05f);

    EXPECT_EQ(aliveBrickCount(game.getState()), aliveBefore);
    EXPECT_EQ(aliveBrickCount(game.getState()), static_cast<std::size_t>(24));
    EXPECT_EQ(game.getState().score, scoreBefore);
    EXPECT_FLOAT_EQ(game.getState().ball.vy, 50.0f);
}

TEST(GameState, CrossingBottomBoundaryTransitionsToLifeLostTransition)
{
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

TEST(GameState, NonCrossingBottomBoundaryRemainsPlaying)
{
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

TEST(GameState, BottomTransitionIsSimulationOnly)
{
    arkanoid::Game game;
    seedPlayingBallState(game, 400.0f, 710.0f, 0.0f, 100.0f);

    game.update(0.20f);

    EXPECT_EQ(game.getState().phase, arkanoid::GamePhase::LifeLostTransition);
    EXPECT_FLOAT_EQ(game.getState().phaseTime, 0.0f);
}

TEST(GameState, BottomLossEventuallyResetsIntoCountdownYellow1)
{
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

TEST(GameState, ScoreIsPreservedAcrossLifeLossReset)
{
    arkanoid::Game game;
    advanceToLifeLostTransitionAfterBrickHit(game);

    const auto scoreBeforeReset = game.getState().score;
    ASSERT_GT(scoreBeforeReset, 0u);

    advanceLifeLostTransitionToCountdownYellow1(game);

    EXPECT_EQ(game.getState().score, scoreBeforeReset);
}

TEST(GameState, RemovedBricksRemainRemovedAcrossLifeLossReset)
{
    arkanoid::Game game;
    advanceToLifeLostTransitionAfterBrickHit(game);

    const std::size_t aliveBeforeReset = aliveBrickCount(game.getState());
    ASSERT_FALSE(game.getState().bricks[0].alive);

    advanceLifeLostTransitionToCountdownYellow1(game);

    EXPECT_FALSE(game.getState().bricks[0].alive);
    EXPECT_EQ(aliveBrickCount(game.getState()), aliveBeforeReset);
}

TEST(GameState, PaddleAndBallRestoreToCanonicalPreServePoseOnReset)
{
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
