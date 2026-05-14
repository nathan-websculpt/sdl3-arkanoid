#include "render/render_frame.hpp"

#include <SDL3/SDL.h>

#include "arkanoid/core/game_geometry.hpp"

namespace arkanoid::render {

namespace {

constexpr float kBallSize = 6.0f;
constexpr float kCountdownIndicatorSize = 20.0f;
constexpr float kCountdownIndicatorY = 560.0f;

bool countdownIndicatorColor(arkanoid::GamePhase phase, Uint8& red, Uint8& green, Uint8& blue) {
    switch (phase) {
    case arkanoid::GamePhase::CountdownYellow1:
    case arkanoid::GamePhase::CountdownYellow2:
        red = 244;
        green = 206;
        blue = 68;
        return true;
    case arkanoid::GamePhase::CountdownGreen:
        red = 92;
        green = 216;
        blue = 112;
        return true;
    case arkanoid::GamePhase::CountdownPause1:
    case arkanoid::GamePhase::CountdownPause2:
    case arkanoid::GamePhase::LaunchDrop:
    case arkanoid::GamePhase::BallReady:
    case arkanoid::GamePhase::Playing:
    case arkanoid::GamePhase::BoardClearedTransition:
    case arkanoid::GamePhase::LifeLostTransition:
    case arkanoid::GamePhase::ResetTransition:
        return false;
    }

    return false;
}

} // namespace

bool renderFrame(SDL_Renderer* renderer, const arkanoid::GameState& state) {
    if (renderer == nullptr) {
        return false;
    }

    if (!SDL_SetRenderDrawColor(renderer, 16, 18, 22, 255)) {
        return false;
    }

    if (!SDL_RenderClear(renderer)) {
        return false;
    }

    if (!SDL_SetRenderDrawColor(renderer, 212, 104, 52, 255)) {
        return false;
    }

    for (const arkanoid::BrickState& brick : state.bricks) {
        if (!brick.alive) {
            continue;
        }

        const SDL_FRect brickRect{brick.x, brick.y, kBrickWidth, kBrickHeight};
        if (!SDL_RenderFillRect(renderer, &brickRect)) {
            return false;
        }
    }

    if (!SDL_SetRenderDrawColor(renderer, 232, 236, 242, 255)) {
        return false;
    }

    const SDL_FRect paddleRect{state.paddle.x - kPaddleHalfWidth, kPaddleTopY,
                               kPaddleHalfWidth * 2.0f, kPaddleHeight};

    if (!SDL_RenderFillRect(renderer, &paddleRect)) {
        return false;
    }

    const SDL_FRect ballRect{state.ball.x - (kBallSize * 0.5f), state.ball.y - (kBallSize * 0.5f),
                             kBallSize, kBallSize};

    if (!SDL_RenderFillRect(renderer, &ballRect)) {
        return false;
    }

    Uint8 countdownRed = 0;
    Uint8 countdownGreen = 0;
    Uint8 countdownBlue = 0;
    if (countdownIndicatorColor(state.phase, countdownRed, countdownGreen, countdownBlue)) {
        if (!SDL_SetRenderDrawColor(renderer, countdownRed, countdownGreen, countdownBlue, 255)) {
            return false;
        }

        const float countdownIndicatorX =
            (static_cast<float>(kWindowWidth) * 0.5f) - (kCountdownIndicatorSize * 0.5f);

        const SDL_FRect countdownRect{countdownIndicatorX, kCountdownIndicatorY,
                                      kCountdownIndicatorSize, kCountdownIndicatorSize};
                                      
        if (!SDL_RenderFillRect(renderer, &countdownRect)) {
            return false;
        }
    }

    return SDL_RenderPresent(renderer);
}

} // namespace arkanoid::render
