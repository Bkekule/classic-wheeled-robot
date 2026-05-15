#include "image_utils.hpp"
#include <cstddef>

namespace robot_control::ball_chaser {

BallRegion findBallRegion(const uint8_t *data, ImageDimensions dims, RgbThreshold threshold) {
    constexpr uint8_t bpp_ = 3;

    int ballXSum_ = 0;
    int ballCount_ = 0;

    for (uint32_t row_ = 0; row_ < dims.height; ++row_) {
        for (uint32_t col_ = 0; col_ < dims.width; ++col_) {
            const size_t idx_ = (row_ * dims.step) + (col_ * bpp_);
            if (data[idx_] >= threshold.r && data[idx_ + 1] >= threshold.g && data[idx_ + 2] >= threshold.b) {
                ballXSum_ += static_cast<int>(col_);
                ++ballCount_;
            }
        }
    }

    if (ballCount_ == 0) {
        return BallRegion::NotFound;
    }

    const int ballCol_ = ballXSum_ / ballCount_;
    const int leftBound_ = static_cast<int>(dims.width) / 3;
    const int rightBound_ = static_cast<int>(dims.width) * 2 / 3;

    if (ballCol_ < leftBound_)
        return BallRegion::Left;
    if (ballCol_ > rightBound_)
        return BallRegion::Right;
    return BallRegion::Center;
}

} // namespace robot_control::ball_chaser
