#include "image_utils.hpp"

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

using namespace robot_control::ball_chaser;

namespace {

/**
 * @brief Builds a flat RGB8 image buffer with all pixels black except one white column.
 *
 * @param dims Image dimensions (width, height, step, bytes_per_pixel).
 * @param whiteCol Column index to fill with white pixels.
 * @return RGB8 image data as a flat vector of bytes.
 */
std::vector<uint8_t> makeImage(ImageDimensions dims, uint32_t whiteCol) {
    std::vector<uint8_t> data_(static_cast<size_t>(dims.step) * dims.height, 0);
    for (uint32_t row_ = 0; row_ < dims.height; ++row_) {
        const size_t idx =
            (static_cast<size_t>(row_) * dims.step) + (static_cast<size_t>(whiteCol) * dims.bytes_per_pixel);
        data_[idx] = 255;
        data_[idx + 1] = 255;
        data_[idx + 2] = 255;
    }
    return data_;
}

} // anonymous namespace

/**
 * @brief Unit tests for ball region detection in camera images.
 *
 * Tests verify that findBallRegion correctly:
 * - Detects white ball pixels meeting RGB thresholds
 * - Returns NotFound when no ball is present
 * - Classifies ball location as Left, Center, or Right based on centroid
 * - Respects RGB threshold limits
 * - Computes centroids correctly for multiple white pixels
 */
class FindBallRegionTest : public ::testing::Test {
  protected:
    /// 10-pixel wide image; thirds: left=[0,2], center=[3,6], right=[7,9].
    static constexpr ImageDimensions m_dims{10, 1, 10 * 3, 3};

    /// Pure white threshold for all RGB channels.
    static constexpr RgbThreshold m_threshold{255, 255, 255};
};

TEST_F(FindBallRegionTest, NoBallReturnsNotFound) {
    std::vector<uint8_t> data_(static_cast<size_t>(m_dims.step) * m_dims.height, 0);
    EXPECT_EQ(findBallRegion(data_.data(), m_dims, m_threshold), BallRegion::NotFound);
}

TEST_F(FindBallRegionTest, BallInLeftThirdReturnsLeft) {
    auto data_ = makeImage(m_dims, 1);
    EXPECT_EQ(findBallRegion(data_.data(), m_dims, m_threshold), BallRegion::Left);
}

TEST_F(FindBallRegionTest, BallInCenterThirdReturnsCenter) {
    auto data_ = makeImage(m_dims, 4);
    EXPECT_EQ(findBallRegion(data_.data(), m_dims, m_threshold), BallRegion::Center);
}

TEST_F(FindBallRegionTest, BallInRightThirdReturnsRight) {
    auto data_ = makeImage(m_dims, 7);
    EXPECT_EQ(findBallRegion(data_.data(), m_dims, m_threshold), BallRegion::Right);
}

TEST_F(FindBallRegionTest, BallBelowThresholdIsIgnored) {
    auto data_ = makeImage(m_dims, 4);
    // Drop one channel below threshold — should not be detected
    data_[(4 * m_dims.bytes_per_pixel) + 1] = 254; // corrupt green channel of the white pixel at col 4
    EXPECT_EQ(findBallRegion(data_.data(), m_dims, m_threshold), BallRegion::NotFound);
}

TEST_F(FindBallRegionTest, AveragedCentroidDeterminesRegion) {
    // Two white pixels: one in left (col 1), one in right (col 7) — centroid lands in center (col 4)
    std::vector<uint8_t> data_(static_cast<size_t>(m_dims.step) * m_dims.height, 0);
    for (uint8_t col : {1U, 7U}) {
        const size_t idx = static_cast<size_t>(col) * m_dims.bytes_per_pixel;
        data_[idx] = data_[idx + 1] = data_[idx + 2] = 255;
    }
    EXPECT_EQ(findBallRegion(data_.data(), m_dims, m_threshold), BallRegion::Center);
}

/** @brief Property 6: Zone ratio boundary computation (Task 1.3).
 *
 * For any image width > 0 and valid zone ratios, findBallRegion classifies a white pixel at column C
 * as Left if C < floor(left_zone_ratio * width), Right if C >= floor(right_zone_ratio * width),
 * and Center otherwise.
 */
RC_GTEST_PROP(FindBallRegionProperty, ZoneRatioBoundaryComputation, ()) {
    const auto width_ = *rc::gen::inRange<uint32_t>(3, 1001);
    const auto leftRatio_ = *rc::gen::map(rc::gen::inRange(1, 49), [](int val) { return val / 100.0; });
    const auto rightRatio_ = *rc::gen::map(rc::gen::inRange(static_cast<int>(leftRatio_ * 100) + 1, 99), [](int val) {
        return val / 100.0;
    });
    const auto col_ = *rc::gen::inRange<uint32_t>(0, width_);

    const uint8_t bpp_ = 3;
    const uint32_t step_ = width_ * bpp_;
    const ImageDimensions dims_{width_, 1, step_, bpp_};
    const RgbThreshold threshold_{255, 255, 255};
    const ZoneRatios zones_{leftRatio_, rightRatio_};

    // Build a 1-row image with a single white pixel at column col_
    std::vector<uint8_t> data_(static_cast<size_t>(step_), 0);
    const size_t pixelIdx_ = static_cast<size_t>(col_) * bpp_;
    data_[pixelIdx_] = 255;
    data_[pixelIdx_ + 1] = 255;
    data_[pixelIdx_ + 2] = 255;

    const auto result_ = findBallRegion(data_.data(), dims_, threshold_, zones_);

    const auto leftBound_ = static_cast<uint32_t>(std::floor(leftRatio_ * width_));
    const auto rightBound_ = static_cast<uint32_t>(std::floor(rightRatio_ * width_));

    if (col_ < leftBound_) {
        RC_ASSERT(result_ == BallRegion::Left);
    } else if (col_ >= rightBound_) {
        RC_ASSERT(result_ == BallRegion::Right);
    } else {
        RC_ASSERT(result_ == BallRegion::Center);
    }
}

/** @brief Property 7: Bytes-per-pixel indexing (Task 1.4).
 *
 * For any valid image with bytes_per_pixel in {3, 4} and a white pixel at a known column,
 * findBallRegion correctly detects the ball regardless of the bytes_per_pixel value by using
 * it for pixel stride calculation.
 */
RC_GTEST_PROP(FindBallRegionProperty, BytesPerPixelIndexing, ()) {
    const auto bpp_ = *rc::gen::element<uint8_t>(3, 4);
    const auto width_ = *rc::gen::inRange<uint32_t>(3, 101);
    const auto col_ = *rc::gen::inRange<uint32_t>(0, width_);

    const uint32_t step_ = width_ * bpp_;
    const ImageDimensions dims_{width_, 1, step_, bpp_};
    const RgbThreshold threshold_{255, 255, 255};
    const ZoneRatios zones_{}; // default 1/3, 2/3

    // Build a 1-row image buffer of size width * bpp, all zeros
    std::vector<uint8_t> data_(static_cast<size_t>(step_), 0);

    // Place white pixel (255,255,255) at offset col_ * bpp (first 3 bytes)
    const size_t pixelIdx_ = static_cast<size_t>(col_) * bpp_;
    data_[pixelIdx_] = 255;
    data_[pixelIdx_ + 1] = 255;
    data_[pixelIdx_ + 2] = 255;

    const auto result_ = findBallRegion(data_.data(), dims_, threshold_, zones_);

    // The ball was placed, it should be detected
    RC_ASSERT(result_ != BallRegion::NotFound);

    // Verify the region matches expected based on default 1/3, 2/3 boundaries
    const auto leftBound_ = static_cast<uint32_t>(std::floor(width_ / 3.0));
    const auto rightBound_ = static_cast<uint32_t>(std::floor(width_ * 2.0 / 3.0));

    if (col_ < leftBound_) {
        RC_ASSERT(result_ == BallRegion::Left);
    } else if (col_ >= rightBound_) {
        RC_ASSERT(result_ == BallRegion::Right);
    } else {
        RC_ASSERT(result_ == BallRegion::Center);
    }
}

/** @brief Test entry point. Runs all GTest cases for image utilities. */
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
