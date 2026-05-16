#include "image_utils.hpp"

#include <gtest/gtest.h>

#include <cstddef>

using namespace robot_control::ball_chaser;

namespace {

/**
 * @brief Builds a flat RGB8 image buffer with all pixels black except one white column.
 *
 * @param dims Image dimensions (width, height, step).
 * @param whiteCol Column index to fill with white pixels.
 * @return RGB8 image data as a flat vector of bytes.
 */
std::vector<uint8_t> makeImage(ImageDimensions dims, uint32_t whiteCol) {
    constexpr uint8_t bpp_ = 3;
    std::vector<uint8_t> data_(static_cast<size_t>(dims.width) * dims.height * bpp_, 0);
    for (uint32_t row_ = 0; row_ < dims.height; ++row_) {
        const size_t idx = (static_cast<size_t>(row_) * dims.width + whiteCol) * bpp_;
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
    /// 10-pixel wide image; thirds: left=[0,2], center=[3,5], right=[6,9].
    static constexpr ImageDimensions m_dims{10, 1, 10 * 3};

    /// Pure white threshold for all RGB channels.
    static constexpr RgbThreshold m_threshold{255, 255, 255};
};

TEST_F(FindBallRegionTest, NoBallReturnsNotFound) {
    std::vector<uint8_t> data_(static_cast<size_t>(m_dims.width) * m_dims.height * 3, 0);
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
    auto data_ = makeImage(m_dims, 6);
    EXPECT_EQ(findBallRegion(data_.data(), m_dims, m_threshold), BallRegion::Right);
}

TEST_F(FindBallRegionTest, BallBelowThresholdIsIgnored) {
    auto data_ = makeImage(m_dims, 4);
    // Drop one channel below threshold — should not be detected
    data_[1] = 254;
    EXPECT_EQ(findBallRegion(data_.data(), m_dims, m_threshold), BallRegion::NotFound);
}

TEST_F(FindBallRegionTest, AveragedCentroidDeterminesRegion) {
    // Two white pixels: one in left (col 1), one in right (col 7) — centroid lands in center (col 4)
    constexpr uint8_t bpp = 3;
    std::vector<uint8_t> data_(static_cast<size_t>(m_dims.width) * m_dims.height * bpp, 0);
    for (uint8_t col : {1U, 7U}) {
        const size_t idx = static_cast<size_t>(col) * bpp;
        data_[idx] = data_[idx + 1] = data_[idx + 2] = 255;
    }
    EXPECT_EQ(findBallRegion(data_.data(), m_dims, m_threshold), BallRegion::Center);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
