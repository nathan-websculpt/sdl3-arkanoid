#include "app/frame_timing.hpp"

#include <SDL3/SDL.h>
#include <cmath>

namespace arkanoid::app {

namespace {

constexpr float kMaxFrameDeltaSeconds = 0.25f;

} // namespace

std::optional<FixedStepTimer> FixedStepTimer::create() {
    const Uint64 performanceFrequency = SDL_GetPerformanceFrequency();
    if (performanceFrequency == 0u) {
        return std::nullopt;
    }

    return FixedStepTimer{static_cast<std::uint64_t>(performanceFrequency),
                          static_cast<std::uint64_t>(SDL_GetPerformanceCounter())};
}

FixedStepTimer::FixedStepTimer(std::uint64_t performanceFrequency, std::uint64_t previousCounter)
    : m_performanceFrequency(performanceFrequency), m_previousCounter(previousCounter) {}

void FixedStepTimer::beginFrame() {
    const Uint64 currentCounter = SDL_GetPerformanceCounter();
    const std::uint64_t currentCount = static_cast<std::uint64_t>(currentCounter);
    const std::uint64_t elapsedCounts = currentCount - m_previousCounter;
    m_previousCounter = currentCount;

    float frameDeltaSeconds = static_cast<float>(static_cast<double>(elapsedCounts) /
                                                 static_cast<double>(m_performanceFrequency));
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
