#include "image_utils.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace robot_control::ball_chaser {

BallRegion findBallRegion(const uint8_t *data, ImageDimensions dims, RgbThreshold threshold, ZoneRatios zones) {
    uint32_t ballXSum_ = 0;
    uint32_t ballCount_ = 0;

    for (uint32_t row_ = 0; row_ < dims.height; ++row_) {
        for (uint32_t col_ = 0; col_ < dims.width; ++col_) {
            const auto idx_ =
                (static_cast<size_t>(row_) * dims.step) + (static_cast<size_t>(col_) * dims.bytes_per_pixel);
            if (data[idx_] >= threshold.r && data[idx_ + 1] >= threshold.g && data[idx_ + 2] >= threshold.b) {
                ballXSum_ += col_;
                ++ballCount_;
            }
        }
    }

    if (ballCount_ == 0) {
        return BallRegion::NotFound;
    }

    const uint32_t ballCol_ = ballXSum_ / ballCount_;
    const auto leftBound_ = static_cast<uint32_t>(std::floor(zones.left * dims.width));
    const auto rightBound_ = static_cast<uint32_t>(std::floor(zones.right * dims.width));

    if (ballCol_ < leftBound_)
        return BallRegion::Left;
    if (ballCol_ >= rightBound_)
        return BallRegion::Right;
    return BallRegion::Center;
}

} // namespace robot_control::ball_chaser
