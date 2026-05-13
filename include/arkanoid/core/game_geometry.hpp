#ifndef ARKANOID_CORE_GAME_GEOMETRY_HPP
#define ARKANOID_CORE_GAME_GEOMETRY_HPP

#include <cstddef>

namespace arkanoid {

inline constexpr int kWindowWidth = 960;
inline constexpr int kWindowHeight = 720;

inline constexpr float kPlayfieldMinX = 0.0f;
inline constexpr float kPlayfieldMinY = 0.0f;
inline constexpr float kPlayfieldMaxX = static_cast<float>(kWindowWidth);
inline constexpr float kPlayfieldMaxY = static_cast<float>(kWindowHeight);
inline constexpr float kPlayfieldCenterX = (kPlayfieldMinX + kPlayfieldMaxX) * 0.5f;

inline constexpr float kPaddleWidth = 160.0f;
inline constexpr float kPaddleHalfWidth = kPaddleWidth * 0.5f;
inline constexpr float kPaddleHeight = 16.0f;
inline constexpr float kPaddleTopY = 620.0f;

inline constexpr int kBrickRows = 3;
inline constexpr int kBrickColumns = 8;
inline constexpr std::size_t kBrickCount = static_cast<std::size_t>(kBrickRows * kBrickColumns);
inline constexpr float kBrickWidth = 100.0f;
inline constexpr float kBrickHeight = 24.0f;
inline constexpr float kBrickHorizontalGap = 8.0f;
inline constexpr float kBrickVerticalGap = 8.0f;
inline constexpr float kBrickStartX = 52.0f;
inline constexpr float kBrickStartY = 80.0f;
inline constexpr float kBrickFieldBottomY =
    kBrickStartY + static_cast<float>(kBrickRows - 1) * (kBrickHeight + kBrickVerticalGap) +
    kBrickHeight;

inline constexpr float kCanonicalPreServeCenterX = kPlayfieldCenterX;
inline constexpr float kCanonicalPreServeBallOffsetY = 200.0f;
inline constexpr float kCanonicalPreServeBallY = kBrickFieldBottomY + kCanonicalPreServeBallOffsetY;

} // namespace arkanoid

#endif
