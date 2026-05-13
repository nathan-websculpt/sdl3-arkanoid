#ifndef ARKANOID_APP_FRAME_TIMING_HPP
#define ARKANOID_APP_FRAME_TIMING_HPP

#include <chrono>

namespace arkanoid::app {

class FixedStepTimer final {
  public:
    [[nodiscard]] static FixedStepTimer create();

    void beginFrame();
    [[nodiscard]] bool hasStep() const;
    void consumeStep();

    [[nodiscard]] static constexpr float fixedStepSeconds() {
        return 1.0f / 120.0f;
    }

  private:
    explicit FixedStepTimer(std::chrono::steady_clock::time_point previousFrameTime);

    std::chrono::steady_clock::time_point m_previousFrameTime;
    float m_accumulatorSeconds{};
};

} // namespace arkanoid::app

#endif
