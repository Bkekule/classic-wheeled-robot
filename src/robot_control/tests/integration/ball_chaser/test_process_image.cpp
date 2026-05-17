#include "process_image_node.hpp"

#include <gtest/gtest.h>
#include <custom_interfaces/srv/drive_to_target.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>

using DriveToTarget = custom_interfaces::srv::DriveToTarget;

namespace {

/**
 * @brief Named parameters for makeImage to prevent adjacent uint32_t arguments being swapped silently.
 */
struct ImageSpec {
    uint32_t width;    ///< Image width in pixels.
    uint32_t whiteCol; ///< Column index to fill with white pixels.
};

/**
 * @brief Builds a flat RGB8 image buffer with all pixels black except one white column.
 *
 * @param spec Image width and the column index to fill with white pixels.
 * @return sensor_msgs::msg::Image with a single white column and all other pixels black.
 */
sensor_msgs::msg::Image makeImage(ImageSpec spec) {
    sensor_msgs::msg::Image msg_;
    msg_.width = spec.width;
    msg_.height = 1;
    msg_.step = spec.width * 3;
    msg_.encoding = "rgb8";
    msg_.data.resize(static_cast<size_t>(spec.width) * 3, 0);
    const size_t idx_ = static_cast<size_t>(spec.whiteCol) * 3;
    msg_.data[idx_] = 255;
    msg_.data[idx_ + 1] = 255;
    msg_.data[idx_ + 2] = 255;
    return msg_;
}

/**
 * @brief The function `makeBlackImage` creates a black image with the specified width in RGB8 encoding.
 *
 * @param width The `width` parameter specifies the width of the image you want to create.
 *
 * @return A sensor_msgs::msg::Image message with the specified width, height of 1, RGB8 encoding, and
 * all pixel values set to black (0,0,0).
 */
sensor_msgs::msg::Image makeBlackImage(uint32_t width) {
    sensor_msgs::msg::Image msg_;
    msg_.width = width;
    msg_.height = 1;
    msg_.step = width * 3;
    msg_.encoding = "rgb8";
    msg_.data.resize(static_cast<size_t>(width) * 3, 0);
    return msg_;
}

/// 10-pixel wide image; thirds: left=[0,2], center=[3,6], right=[7,9]
constexpr uint32_t k_width = 10;
} // anonymous namespace

/**
 * @brief Integration tests for the ProcessImage ROS 2 client node.
 *
 * Tests verify that ProcessImage correctly:
 * - Calls DriveToTarget with angular velocity when the ball is left or right
 * - Calls DriveToTarget with linear velocity when the ball is centered
 * - Sends a stop command exactly once when the ball disappears after movement
 * - Does not call the service when no ball is detected and the robot was already stopped
 */
class ProcessImageTest : public ::testing::Test {
  protected:
    /**
     * @brief The SetUp function creates a stub service node, a publisher node, and a client node, and waits for
     * the client node to discover the stub service before publishing images.
     */
    void SetUp() override {
        // Stub service node — captures the most recent request sent by ProcessImage
        m_stubNode = rclcpp::Node::make_shared("drive_stub");
        // NOLINTBEGIN(performance-unnecessary-value-param)
        m_service = m_stubNode->create_service<DriveToTarget>(
            "/ball_chaser/command_robot",
            [this](const DriveToTarget::Request::SharedPtr req, DriveToTarget::Response::SharedPtr res) {
                m_lastRequest = *req;
                ++m_callCount;
                res->msg_feedback = "ok";
            }
        );
        // NOLINTEND(performance-unnecessary-value-param)

        // Publisher node — injects synthetic images into the pipeline
        m_pubNode = rclcpp::Node::make_shared("image_pub");
        m_pub = m_pubNode->create_publisher<sensor_msgs::msg::Image>("/camera/rgb/image_raw", 10);

        m_client = std::make_shared<robot_control::ball_chaser::ProcessImage>();

        m_executor.add_node(m_stubNode);
        m_executor.add_node(m_pubNode);
        m_executor.add_node(m_client);

        // Spin until the stub service is discoverable
        auto probe_ = m_pubNode->create_client<DriveToTarget>("/ball_chaser/command_robot");
        while (!probe_->wait_for_service(std::chrono::milliseconds(10))) {
            m_executor.spin_some();
        }
    }

    /**
     * @brief The `TearDown` function removes specific nodes from an executor.
     */
    void TearDown() override {
        m_executor.remove_node(m_stubNode);
        m_executor.remove_node(m_pubNode);
        m_executor.remove_node(m_client);
    }

    /**
     * @brief The function `spinFor` spins the executor for a specified duration using a steady clock until the
     * deadline is reached.
     *
     * @param duration The `duration` parameter in the `spinFor` function represents the amount of time
     * for which the function will keep spinning the executor. It is of type `std::chrono::milliseconds`,
     * which means it expects a duration specified in milliseconds.
     */
    void spinFor(std::chrono::milliseconds duration) {
        auto deadline_ = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < deadline_) {
            m_executor.spin_some();
        }
    }

    /**
     * @brief The function `publishAndWaitForCall` publishes an image and waits for a specified number of expected
     * calls before timing out.
     *
     * @param img The `img` parameter is of type `sensor_msgs::msg::Image`, which is a message type used in
     * ROS (Robot Operating System) for representing images. It likely contains data such as image height,
     * width, encoding type, and pixel data. In this function, the `img` parameter
     * @param expectedCallCount The `expectedCallCount` parameter represents the number of times a certain
     * function or method is expected to be called before the `publishAndWaitForCall` function completes
     * its execution. This parameter is used to wait until the expected number of calls have been made
     * before proceeding further in the code.
     */
    void publishAndWaitForCall(const sensor_msgs::msg::Image &img, uint8_t expectedCallCount) {
        m_pub->publish(img);
        auto deadline_ = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (m_callCount < expectedCallCount) {
            ASSERT_LT(std::chrono::steady_clock::now(), deadline_)
                << "Timed out waiting for drive call #" << expectedCallCount;
            m_executor.spin_some();
        }
    }

    std::shared_ptr<rclcpp::Node> m_stubNode; ///< Stub node hosting the fake DriveToTarget service.
    std::shared_ptr<rclcpp::Node> m_pubNode;  ///< Publisher node that sends test images.
    std::shared_ptr<robot_control::ball_chaser::ProcessImage> m_client; ///< ProcessImage node under test.
    rclcpp::Service<DriveToTarget>::SharedPtr m_service;         ///< Fake DriveToTarget service that captures requests.
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr m_pub; ///< Publisher for the test image topic.
    rclcpp::executors::SingleThreadedExecutor m_executor;        ///< Executor to spin all nodes during tests.

    DriveToTarget::Request m_lastRequest; ///< Most recent request received by the fake service.
    uint8_t m_callCount{0};               ///< Number of times the fake service has been called.
};

TEST_F(ProcessImageTest, BallOnLeftCommandsTurnLeft) {
    publishAndWaitForCall(makeImage({k_width, 1}), 1);

    ASSERT_EQ(m_callCount, 1);
    EXPECT_DOUBLE_EQ(m_lastRequest.linear_x, 0.0);
    EXPECT_GT(m_lastRequest.angular_z, 0.0);
}

TEST_F(ProcessImageTest, BallInCenterCommandsDriveForward) {
    publishAndWaitForCall(makeImage({k_width, 4}), 1);

    ASSERT_EQ(m_callCount, 1);
    EXPECT_GT(m_lastRequest.linear_x, 0.0);
    EXPECT_DOUBLE_EQ(m_lastRequest.angular_z, 0.0);
}

TEST_F(ProcessImageTest, BallOnRightCommandsTurnRight) {
    publishAndWaitForCall(makeImage({k_width, 7}), 1);

    ASSERT_EQ(m_callCount, 1);
    EXPECT_DOUBLE_EQ(m_lastRequest.linear_x, 0.0);
    EXPECT_LT(m_lastRequest.angular_z, 0.0);
}

TEST_F(ProcessImageTest, NoBallAfterMovementCommandsStop) {
    // First frame: ball detected — robot starts moving
    publishAndWaitForCall(makeImage({k_width, 4}), 1);
    ASSERT_EQ(m_callCount, 1);

    // Second frame: no ball — robot should receive exactly one stop command
    publishAndWaitForCall(makeBlackImage(k_width), 2);
    ASSERT_EQ(m_callCount, 2);
    EXPECT_DOUBLE_EQ(m_lastRequest.linear_x, 0.0);
    EXPECT_DOUBLE_EQ(m_lastRequest.angular_z, 0.0);
}

TEST_F(ProcessImageTest, NoBallWhileAlreadyStoppedDoesNotCallService) {
    // Robot was never moving — two consecutive empty frames should produce no calls
    publishAndWaitForCall(makeBlackImage(k_width), 0);
    spinFor(std::chrono::milliseconds(200));
    publishAndWaitForCall(makeBlackImage(k_width), 0);
    spinFor(std::chrono::milliseconds(200));

    EXPECT_EQ(m_callCount, 0);
}

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    int result_ = RUN_ALL_TESTS();
    rclcpp::shutdown();
    return result_;
}
