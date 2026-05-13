#ifndef ARKANOID_APP_RUN_MODE_HPP
#define ARKANOID_APP_RUN_MODE_HPP

namespace arkanoid::app {

enum class RunMode { Normal, ReleaseStartupSmoke, Invalid };

[[nodiscard]] RunMode parseRunMode(int argc, char* argv[]);

} // namespace arkanoid::app

#endif
