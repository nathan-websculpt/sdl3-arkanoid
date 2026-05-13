#include "app/frame_timing.hpp"

#include <chrono>
#include <cmath>

namespace arkanoid::app {

namespace {

constexpr float kMaxFrameDeltaSeconds = 0.25f;
using SecondsF = std::chrono::duration<float>;

} // namespace

FixedStepTimer FixedStepTimer::create() {
    return FixedStepTimer{std::chrono::steady_clock::now()};
}

FixedStepTimer::FixedStepTimer(std::chrono::steady_clock::time_point previousFrameTime)
    : m_previousFrameTime(previousFrameTime) {}

void FixedStepTimer::beginFrame() {
    const auto currentFrameTime = std::chrono::steady_clock::now();
    float frameDeltaSeconds = SecondsF{currentFrameTime - m_previousFrameTime}.count();
    m_previousFrameTime = currentFrameTime;

    if (!std::isfinite(frameDeltaSeconds) || frameDeltaSeconds < 0.0f) {
        frameDeltaSeconds = 0.0f;
    }
    if (frameDeltaSeconds > kMaxFrameDeltaSeconds) {
        frameDeltaSeconds = kMaxFrameDeltaSeconds;
    }
    m_accumulatorSeconds += frameDeltaSeconds;
}

bool FixedStepTimer::hasStep() const {
    return m_accumulatorSeconds >= fixedStepSeconds();
}

void FixedStepTimer::consumeStep() {
    m_accumulatorSeconds -= fixedStepSeconds();
}

} // namespace arkanoid::app
