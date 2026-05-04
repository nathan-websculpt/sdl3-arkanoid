#ifndef ARKANOID_APP_FRAME_TIMING_HPP
#define ARKANOID_APP_FRAME_TIMING_HPP

#include <cstdint>
#include <optional>

namespace arkanoid::app {

class FixedStepTimer final {
  public:
    static std::optional<FixedStepTimer> create();

    void beginFrame();
    bool hasStep() const;
    void consumeStep();

    static constexpr float fixedStepSeconds() {
        return 1.0f / 120.0f;
    }

  private:
    FixedStepTimer(std::uint64_t performanceFrequency, std::uint64_t previousCounter);

    std::uint64_t m_performanceFrequency{};
    std::uint64_t m_previousCounter{};
    float m_accumulatorSeconds{};
};

} // namespace arkanoid::app

#endif
