#ifndef ARKANOID_APP_INPUT_HPP
#define ARKANOID_APP_INPUT_HPP

namespace arkanoid::app {

struct InputState {
    bool moveLeftHeld{};
    bool moveRightHeld{};
    bool serveHeld{};
};

InputState readInputState();

} // namespace arkanoid::app

#endif
