#include "app/run_mode.hpp"

#include <string_view>

namespace arkanoid::app {

RunMode parseRunMode(int argc, char* argv[]) {
    if (argc <= 1) {
        return RunMode::Normal;
    }

    if (argc == 2 && argv != nullptr && argv[1] != nullptr &&
        std::string_view(argv[1]) == "--release-startup-smoke") {
        return RunMode::ReleaseStartupSmoke;
    }

    return RunMode::Invalid;
}

} // namespace arkanoid::app
