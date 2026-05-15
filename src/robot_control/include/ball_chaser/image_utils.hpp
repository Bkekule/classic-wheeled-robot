#pragma once

#include <cstdint>

namespace robot_control::ball_chaser {

struct RgbThreshold {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct ImageDimensions {
    uint32_t width;
    uint32_t height;
    uint32_t step;
};

enum class BallRegion : std::uint8_t { Left, Center, Right, NotFound };

BallRegion findBallRegion(const uint8_t *data, ImageDimensions dims, RgbThreshold threshold);

} // namespace robot_control::ball_chaser
