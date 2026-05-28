#pragma once

#include <cstdint>

namespace robot_control::ball_chaser {

/**
 * @brief RGB color threshold values for ball detection.
 */
struct RgbThreshold {
    uint8_t r; ///< Red channel threshold.
    uint8_t g; ///< Green channel threshold.
    uint8_t b; ///< Blue channel threshold.
};

/**
 * @brief Image dimensions and stride information.
 */
struct ImageDimensions {
    uint32_t width;          ///< Image width in pixels.
    uint32_t height;         ///< Image height in pixels.
    uint32_t step;           ///< Bytes per row (stride).
    uint8_t bytes_per_pixel; ///< Bytes per pixel (e.g., 3 for RGB8, 4 for RGBA8).
};

/**
 * @brief Zone boundary ratios for ball region classification.
 */
struct ZoneRatios {
    double left = 1.0 / 3.0;  ///< Left zone boundary (pixels with centroid < left*width are Left).
    double right = 2.0 / 3.0; ///< Right zone boundary (pixels with centroid >= right*width are Right).
};

/**
 * @brief Detected ball region in the image.
 */
enum class BallRegion : std::uint8_t { Left, Center, Right, NotFound };

/**
 * @brief Detects the ball region within an image based on RGB thresholds.
 *
 * @param data Pointer to raw image data (RGB format).
 * @param dims Image dimensions and stride.
 * @param threshold RGB color threshold for ball detection.
 * @param zones Zone boundary ratios for region classification.
 * @return BallRegion indicating where the ball was detected, or NotFound if not detected.
 */
BallRegion findBallRegion(const uint8_t *data, ImageDimensions dims, RgbThreshold threshold, ZoneRatios zones = {});

} // namespace robot_control::ball_chaser
