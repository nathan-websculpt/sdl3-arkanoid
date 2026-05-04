#include "app/application.hpp"
#include "app/run_mode.hpp"

int main(int argc, char* argv[]) {
    const arkanoid::app::RunMode runMode = arkanoid::app::parseRunMode(argc, argv);

    if (runMode == arkanoid::app::RunMode::Invalid) {
        return 1;
    }

    return arkanoid::app::runApplication(runMode);
}
