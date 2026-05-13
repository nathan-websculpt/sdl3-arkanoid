#ifndef ARKANOID_RENDER_RENDER_FRAME_HPP
#define ARKANOID_RENDER_RENDER_FRAME_HPP

#include "arkanoid/sim/game_state.hpp"

struct SDL_Renderer;

namespace arkanoid::render {

[[nodiscard]] bool renderFrame(SDL_Renderer* renderer, const arkanoid::GameState& state);

} // namespace arkanoid::render

#endif
