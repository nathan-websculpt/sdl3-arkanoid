#include "app/application.hpp"

#include <SDL3/SDL.h>
#include <memory>
#include <optional>

#include "app/frame_timing.hpp"
#include "app/input.hpp"
#include "arkanoid/core/game.hpp"
#include "render/render_frame.hpp"

namespace arkanoid::app {

namespace {

constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 720;

struct SdlQuitGuard final {
    SdlQuitGuard() = default;

    // SDL_Quit is a process-wide lifetime action; this guard must not be copied or moved
    SdlQuitGuard(const SdlQuitGuard&) = delete;
    SdlQuitGuard& operator=(const SdlQuitGuard&) = delete;
    SdlQuitGuard(SdlQuitGuard&&) = delete;
    SdlQuitGuard& operator=(SdlQuitGuard&&) = delete;

    ~SdlQuitGuard() {
        SDL_Quit();
    }
};

bool pollQuitRequested() {
    bool quitRequested = false;
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            quitRequested = true;
        }
    }

    return quitRequested;
}

} // namespace

int runApplication(RunMode runMode) {
    if (runMode == RunMode::Invalid) {
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return 1;
    }

    SdlQuitGuard sdlQuitGuard{};

    const bool releaseStartupSmoke = runMode == RunMode::ReleaseStartupSmoke;
    const auto windowFlags = releaseStartupSmoke ? SDL_WINDOW_HIDDEN : 0;
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
        SDL_CreateWindow("Arkanoid Demo", kWindowWidth, kWindowHeight, windowFlags),
        SDL_DestroyWindow);
    if (!window) {
        return 1;
    }

    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
        SDL_CreateRenderer(window.get(), nullptr), SDL_DestroyRenderer);
    if (!renderer) {
        return 1;
    }
    if (!SDL_SetRenderVSync(renderer.get(), 1)) {
        SDL_Log("Failed to enable renderer VSync: %s", SDL_GetError());
        return 1;
    }

    arkanoid::Game game;
    if (releaseStartupSmoke) {
        return arkanoid::render::renderFrame(renderer.get(), game.getState()) ? 0 : 1;
    }

    std::optional<FixedStepTimer> timer = FixedStepTimer::create();
    if (!timer.has_value()) {
        return 1;
    }

    bool running = true;
    while (running) {
        if (pollQuitRequested()) {
            running = false;
        }
        if (!running) {
            break;
        }

        const InputState input = readInputState();

        timer->beginFrame();
        while (timer->hasStep()) {
            game.setInput(input.moveLeftHeld, input.moveRightHeld, input.serveHeld);
            game.update(FixedStepTimer::fixedStepSeconds());
            timer->consumeStep();
        }

        if (!arkanoid::render::renderFrame(renderer.get(), game.getState())) {
            return 1;
        }
    }

    return 0;
}

} // namespace arkanoid::app
