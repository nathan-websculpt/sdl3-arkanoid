#include "arkanoid/core/game.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

namespace arkanoid {

namespace {

constexpr float kCountdownYellow1Duration = 0.25f;
constexpr float kCountdownPause1Duration = 0.35f;
constexpr float kCountdownYellow2Duration = 0.25f;
constexpr float kCountdownPause2Duration = 0.35f;
constexpr float kCountdownGreenDuration = 0.10f;
constexpr float kLaunchDropSpawnY = 368.0f;
constexpr float kLaunchDropContactY = 620.0f;
constexpr float kLaunchDropVelocityY = 400.0f;
constexpr float kServeVelocityY = -400.0f;
constexpr float kPaddleSpeed = 400.0f;
constexpr float kPaddleHalfWidth = 80.0f;
constexpr float kPaddleMaxHorizontalSpeedFactor = 0.85f;
constexpr float kPlayfieldMinX = 0.0f;
constexpr float kPlayfieldMaxX = 960.0f;
constexpr float kPlayfieldMinY = 0.0f;
constexpr float kPlayfieldMaxY = 720.0f;
constexpr int kBrickRows = 3;
constexpr int kBrickColumns = 8;
constexpr float kBrickWidth = 100.0f;
constexpr float kBrickHeight = 24.0f;
constexpr float kBrickHorizontalGap = 8.0f;
constexpr float kBrickVerticalGap = 8.0f;
constexpr float kBrickStartX = 52.0f;
constexpr float kBrickStartY = 80.0f;
constexpr float kCanonicalPreServeCenterX = (kPlayfieldMinX + kPlayfieldMaxX) * 0.5f;
constexpr float kBrickFieldBottomY = kBrickStartY + static_cast<float>(kBrickRows - 1) * (kBrickHeight + kBrickVerticalGap) + kBrickHeight;
constexpr float kCanonicalPreServeBallOffsetY = 200.0f;
constexpr float kCanonicalPreServeBallY = kBrickFieldBottomY + kCanonicalPreServeBallOffsetY;
constexpr std::array<std::string_view, static_cast<std::size_t>(kBrickRows)> kBrickLayout{
    "XXXXXXXX",
    "XXXXXXXX",
    "XXXXXXXX",
};

constexpr bool isValidBrickLayout() {
    for (const std::string_view row : kBrickLayout) {
        if (row.size() != static_cast<std::size_t>(kBrickColumns)) {
            return false;
        }

        for (const char cell : row) {
            if (cell != 'X' && cell != '.') {
                return false;
            }
        }
    }

    return true;
}

static_assert(isValidBrickLayout(), "brick layout must use only X/. and match configured dimensions");

std::array<BrickState, 24> makeInitialBricks() {
    std::array<BrickState, 24> bricks{};
    for (int row = 0; row < kBrickRows; ++row) {
        for (int column = 0; column < kBrickColumns; ++column) {
            const int index = (row * kBrickColumns) + column;
            const std::size_t rowIndex = static_cast<std::size_t>(row);
            const std::size_t columnIndex = static_cast<std::size_t>(column);
            BrickState& brick = bricks[static_cast<std::size_t>(index)];
            brick.x = kBrickStartX + static_cast<float>(column) * (kBrickWidth + kBrickHorizontalGap);
            brick.y = kBrickStartY + static_cast<float>(row) * (kBrickHeight + kBrickVerticalGap);
            brick.alive = kBrickLayout[rowIndex][columnIndex] == 'X';
        }
    }

    return bricks;
}

int movementDirection(bool moveLeftHeld, bool moveRightHeld) {
    if (moveLeftHeld == moveRightHeld) {
        return 0;
    }

    return moveLeftHeld ? -1 : 1;
}

bool isPaddleMovementEnabled(GamePhase phase) {
    switch (phase) {
        case GamePhase::CountdownGreen:
        case GamePhase::LaunchDrop:
        case GamePhase::BallReady:
        case GamePhase::Playing:
            return true;
        case GamePhase::CountdownYellow1:
        case GamePhase::CountdownPause1:
        case GamePhase::CountdownYellow2:
        case GamePhase::CountdownPause2:
        case GamePhase::LifeLostTransition:
        case GamePhase::ResetTransition:
            return false;
    }

    return false;
}

void applyCanonicalPreServePose(GameState& state) {
    state.paddle.x = kCanonicalPreServeCenterX;
    state.ball.x = kCanonicalPreServeCenterX;
    state.ball.y = kCanonicalPreServeBallY;
    state.ball.vx = 0.0f;
    state.ball.vy = 0.0f;
}

} // namespace

Game::Game() {
    m_state.bricks = makeInitialBricks();
    applyCanonicalPreServePose(m_state);
    m_state.phase = GamePhase::CountdownYellow1;
    m_state.phaseTime = 0.0f;
}

void Game::setInput(bool moveLeftHeld, bool moveRightHeld, bool serveHeld) {
    m_moveLeftHeld = moveLeftHeld;
    m_moveRightHeld = moveRightHeld;
    m_serveHeld = serveHeld;
}

void Game::update(float dt) {
    if (!std::isfinite(dt) || dt <= 0.0f) {
        return;
    }

    const int direction = movementDirection(m_moveLeftHeld, m_moveRightHeld);
    if (isPaddleMovementEnabled(m_state.phase) && direction != 0) {
        m_state.paddle.x += static_cast<float>(direction) * kPaddleSpeed * dt;
    }
    m_state.paddle.x = std::clamp(m_state.paddle.x, kPlayfieldMinX, kPlayfieldMaxX);

    m_state.phaseTime += dt;

    switch (m_state.phase) {
        case GamePhase::CountdownYellow1:
            if (m_state.phaseTime >= kCountdownYellow1Duration) {
                m_state.phase = GamePhase::CountdownPause1;
                m_state.phaseTime = 0.0f;
            }
            break;
        case GamePhase::CountdownPause1:
            if (m_state.phaseTime >= kCountdownPause1Duration) {
                m_state.phase = GamePhase::CountdownYellow2;
                m_state.phaseTime = 0.0f;
            }
            break;
        case GamePhase::CountdownYellow2:
            if (m_state.phaseTime >= kCountdownYellow2Duration) {
                m_state.phase = GamePhase::CountdownPause2;
                m_state.phaseTime = 0.0f;
            }
            break;
        case GamePhase::CountdownPause2:
            if (m_state.phaseTime >= kCountdownPause2Duration) {
                m_state.phase = GamePhase::CountdownGreen;
                m_state.phaseTime = 0.0f;
            }
            break;
        case GamePhase::CountdownGreen:
            if (m_state.phaseTime >= kCountdownGreenDuration) {
                m_state.ball.x = m_state.paddle.x;
                m_state.ball.y = kLaunchDropSpawnY;
                m_state.ball.vx = 0.0f;
                m_state.ball.vy = kLaunchDropVelocityY;
                m_state.phase = GamePhase::LaunchDrop;
                m_state.phaseTime = 0.0f;
            }
            break;
        case GamePhase::LaunchDrop:
            m_state.ball.x = m_state.paddle.x;
            m_state.ball.y += m_state.ball.vy * dt;
            if (m_state.ball.y >= kLaunchDropContactY) {
                m_state.ball.x = m_state.paddle.x;
                m_state.ball.y = kLaunchDropContactY;
                m_state.ball.vx = 0.0f;
                m_state.ball.vy = 0.0f;
                m_state.phase = GamePhase::BallReady;
                m_state.phaseTime = 0.0f;
            }
            break;
        case GamePhase::BallReady: {
            m_state.ball.x = m_state.paddle.x;
            m_state.ball.y = kLaunchDropContactY;
            m_state.ball.vx = 0.0f;
            m_state.ball.vy = 0.0f;

            if (m_serveHeld && !m_previousServeHeld) {
                m_state.ball.vx = 0.0f;
                m_state.ball.vy = kServeVelocityY;
                m_state.phase = GamePhase::Playing;
                m_state.phaseTime = 0.0f;
            }
            break;
        }
        case GamePhase::Playing: {
            const float previousBallY = m_state.ball.y;
            m_state.ball.x += m_state.ball.vx * dt;
            m_state.ball.y += m_state.ball.vy * dt;

            if (m_state.ball.x <= kPlayfieldMinX) {
                m_state.ball.x = kPlayfieldMinX;
                m_state.ball.vx = std::abs(m_state.ball.vx);
            }

            if (m_state.ball.x >= kPlayfieldMaxX) {
                m_state.ball.x = kPlayfieldMaxX;
                m_state.ball.vx = -std::abs(m_state.ball.vx);
            }

            if (m_state.ball.y <= kPlayfieldMinY) {
                m_state.ball.y = kPlayfieldMinY;
                m_state.ball.vy = std::abs(m_state.ball.vy);
            }

            const bool ballMovingDownward = m_state.ball.vy > 0.0f;
            const bool crossedPaddleTop = previousBallY < kLaunchDropContactY && m_state.ball.y >= kLaunchDropContactY;
            const float paddleMinX = m_state.paddle.x - kPaddleHalfWidth;
            const float paddleMaxX = m_state.paddle.x + kPaddleHalfWidth;
            const bool ballWithinPaddleCoverage = m_state.ball.x >= paddleMinX && m_state.ball.x <= paddleMaxX;

            if (ballMovingDownward && crossedPaddleTop && ballWithinPaddleCoverage) {
                m_state.ball.y = kLaunchDropContactY;
                const float speed = std::hypot(m_state.ball.vx, m_state.ball.vy);
                const float rawOffset = (m_state.ball.x - m_state.paddle.x) / kPaddleHalfWidth;
                const float offset = std::clamp(rawOffset, -1.0f, 1.0f);
                m_state.ball.vx = speed * kPaddleMaxHorizontalSpeedFactor * offset;
                const float verticalSpeedSquared = std::max(0.0f, (speed * speed) - (m_state.ball.vx * m_state.ball.vx));
                m_state.ball.vy = -std::sqrt(verticalSpeedSquared);
            }

            for (BrickState& brick : m_state.bricks) {
                if (!brick.alive) {
                    continue;
                }

                const float brickLeft = brick.x;
                const float brickRight = brick.x + kBrickWidth;
                const bool overlapsBrickHorizontally = m_state.ball.x >= brickLeft && m_state.ball.x <= brickRight;
                if (!overlapsBrickHorizontally) {
                    continue;
                }

                const float brickTop = brick.y;
                const float brickBottom = brick.y + kBrickHeight;
                const bool crossedBrickTop = previousBallY < brickTop && m_state.ball.y >= brickTop;
                const bool crossedBrickBottom = previousBallY > brickBottom && m_state.ball.y <= brickBottom;
                if (!crossedBrickTop && !crossedBrickBottom) {
                    continue;
                }

                brick.alive = false;
                m_state.ball.vy = -m_state.ball.vy;
                m_state.score += 1;
                break;
            }

            const bool crossedBottomBoundary = previousBallY < kPlayfieldMaxY && m_state.ball.y >= kPlayfieldMaxY;
            if (crossedBottomBoundary) {
                m_state.phase = GamePhase::LifeLostTransition;
                m_state.phaseTime = 0.0f;
            }
            break;
        }
        case GamePhase::LifeLostTransition:
            m_state.phase = GamePhase::ResetTransition;
            m_state.phaseTime = 0.0f;
            break;
        case GamePhase::ResetTransition:
            applyCanonicalPreServePose(m_state);
            m_state.phase = GamePhase::CountdownYellow1;
            m_state.phaseTime = 0.0f;
            break;
    }

    m_previousServeHeld = m_serveHeld;
}

const GameState& Game::getState() const {
    return m_state;
}

} // namespace arkanoid
