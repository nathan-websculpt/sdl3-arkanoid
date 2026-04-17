#include <cmath>
#include <memory>

#include <SDL3/SDL.h>

#include "arkanoid/core/game.hpp"

namespace {

constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 720;
constexpr float kFixedStepSeconds = 1.0f / 120.0f;
constexpr float kMaxFrameDeltaSeconds = 0.25f;
constexpr float kPaddleHalfWidth = 80.0f;
constexpr float kPaddleHeight = 16.0f;
constexpr float kPaddleTopY = 620.0f;
constexpr float kBrickWidth = 100.0f;
constexpr float kBrickHeight = 24.0f;
constexpr float kBallSize = 6.0f;
constexpr float kCountdownIndicatorSize = 20.0f;
constexpr float kCountdownIndicatorY = 560.0f;

bool isScancodePressed(const bool* keyboardState, int keyCount, SDL_Scancode scancode) {
    if (keyboardState == nullptr || keyCount <= 0) {
        return false;
    }

    const int scancodeIndex = static_cast<int>(scancode);
    if (scancodeIndex < 0 || scancodeIndex >= keyCount) {
        return false;
    }

    return keyboardState[scancodeIndex];
}

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
        case arkanoid::GamePhase::LifeLostTransition:
        case arkanoid::GamePhase::ResetTransition:
            return false;
    }

    return false;
}

} // namespace

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return 1;
    }

    struct SdlQuitGuard final {
        ~SdlQuitGuard() {
            SDL_Quit();
        }
    } sdlQuitGuard{};

    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
        SDL_CreateWindow("Arkanoid Demo", kWindowWidth, kWindowHeight, 0), SDL_DestroyWindow);
    if (!window) {
        return 1;
    }

    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(SDL_CreateRenderer(window.get(), nullptr), SDL_DestroyRenderer);
    if (!renderer) {
        return 1;
    }

    const Uint64 performanceFrequency = SDL_GetPerformanceFrequency();
    if (performanceFrequency == 0u) {
        return 1;
    }

    arkanoid::Game game;
    Uint64 previousCounter = SDL_GetPerformanceCounter();
    float accumulatorSeconds = 0.0f;
    bool running = true;

    while (running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }
        if (!running) {
            break;
        }

        int keyCount = 0;
        const bool* keyboardState = SDL_GetKeyboardState(&keyCount);
        const bool moveLeftHeld = isScancodePressed(keyboardState, keyCount, SDL_SCANCODE_LEFT);
        const bool moveRightHeld = isScancodePressed(keyboardState, keyCount, SDL_SCANCODE_RIGHT);
        const bool serveHeld = isScancodePressed(keyboardState, keyCount, SDL_SCANCODE_SPACE);

        const Uint64 currentCounter = SDL_GetPerformanceCounter();
        const Uint64 elapsedCounts = currentCounter - previousCounter;
        previousCounter = currentCounter;

        float frameDeltaSeconds = static_cast<float>(static_cast<double>(elapsedCounts) / static_cast<double>(performanceFrequency));
        if (!std::isfinite(frameDeltaSeconds) || frameDeltaSeconds < 0.0f) {
            frameDeltaSeconds = 0.0f;
        }
        if (frameDeltaSeconds > kMaxFrameDeltaSeconds) {
            frameDeltaSeconds = kMaxFrameDeltaSeconds;
        }
        accumulatorSeconds += frameDeltaSeconds;

        while (accumulatorSeconds >= kFixedStepSeconds) {
            game.setInput(moveLeftHeld, moveRightHeld, serveHeld);
            game.update(kFixedStepSeconds);
            accumulatorSeconds -= kFixedStepSeconds;
        }

        const arkanoid::GameState& state = game.getState();

        if (!SDL_SetRenderDrawColor(renderer.get(), 16, 18, 22, 255)) {
            return 1;
        }
        if (!SDL_RenderClear(renderer.get())) {
            return 1;
        }

        if (!SDL_SetRenderDrawColor(renderer.get(), 212, 104, 52, 255)) {
            return 1;
        }
        for (const arkanoid::BrickState& brick : state.bricks) {
            if (!brick.alive) {
                continue;
            }

            const SDL_FRect brickRect{brick.x, brick.y, kBrickWidth, kBrickHeight};
            if (!SDL_RenderFillRect(renderer.get(), &brickRect)) {
                return 1;
            }
        }

        if (!SDL_SetRenderDrawColor(renderer.get(), 232, 236, 242, 255)) {
            return 1;
        }
        const SDL_FRect paddleRect{state.paddle.x - kPaddleHalfWidth, kPaddleTopY, kPaddleHalfWidth * 2.0f, kPaddleHeight};
        if (!SDL_RenderFillRect(renderer.get(), &paddleRect)) {
            return 1;
        }

        const SDL_FRect ballRect{state.ball.x - (kBallSize * 0.5f), state.ball.y - (kBallSize * 0.5f), kBallSize, kBallSize};
        if (!SDL_RenderFillRect(renderer.get(), &ballRect)) {
            return 1;
        }

        Uint8 countdownRed = 0;
        Uint8 countdownGreen = 0;
        Uint8 countdownBlue = 0;
        if (countdownIndicatorColor(state.phase, countdownRed, countdownGreen, countdownBlue)) {
            if (!SDL_SetRenderDrawColor(renderer.get(), countdownRed, countdownGreen, countdownBlue, 255)) {
                return 1;
            }

            const float countdownIndicatorX = (static_cast<float>(kWindowWidth) * 0.5f) - (kCountdownIndicatorSize * 0.5f);
            const SDL_FRect countdownRect{countdownIndicatorX, kCountdownIndicatorY, kCountdownIndicatorSize, kCountdownIndicatorSize};
            if (!SDL_RenderFillRect(renderer.get(), &countdownRect)) {
                return 1;
            }
        }

        if (!SDL_RenderPresent(renderer.get())) {
            return 1;
        }
    }

    return 0;
}
