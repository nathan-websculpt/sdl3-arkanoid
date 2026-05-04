#include "app/input.hpp"

#include <SDL3/SDL.h>

namespace arkanoid::app {

namespace {

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

} // namespace

InputState readInputState() {
    int keyCount = 0;
    const bool* keyboardState = SDL_GetKeyboardState(&keyCount);

    return InputState{
        isScancodePressed(keyboardState, keyCount, SDL_SCANCODE_LEFT),
        isScancodePressed(keyboardState, keyCount, SDL_SCANCODE_RIGHT),
        isScancodePressed(keyboardState, keyCount, SDL_SCANCODE_SPACE),
    };
}

} // namespace arkanoid::app
