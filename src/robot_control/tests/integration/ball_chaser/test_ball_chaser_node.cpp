/**
 * @file test_ball_chaser_node.cpp
 * @brief Integration tests for BallChaserNode.
 *
 * Verifies timing, state transitions, and velocity publishing behavior of the
 * unified ball chaser node using synthetic camera images.
 *
 * Requirements validated:
 * - 7.1: Velocity published within 100ms of ball frame
 * - 7.2: Idle→Tracking on ball detection
 * - 7.3: Tracking→Lost on ball disappearance
 * - 7.4: Lost→Idle after lost_timeout with zero-velocity
 * - 7.5: Lost→Tracking on ball reappearance before timeout
 * - 7.6: State string values limited to "Idle", "Tracking", "Lost"
 * - 7.7: No velocity published in Idle with no ball for 500ms
 */

#include "ball_chaser_node.hpp"

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <std_msgs/msg/string.hpp>

using namespace std::chrono_literals;

namespace {

// ─── Helper functions ────────────────────────────────────────────────────────

/**
 * @brief Creates a sensor_msgs::msg::Image with one white column on black.
 *
 * @param width Image width in pixels.
 * @param whiteCol Column index to fill with white pixels (all rows).
 * @return Synthetic RGB8 image message.
 */
sensor_msgs::msg::Image makeImage(uint32_t width, uint32_t whiteCol) {
    sensor_msgs::msg::Image img;
    img.encoding = "rgb8";
    img.width = width;
    img.height = 1;
    img.step = width * 3;
    img.data.resize(static_cast<size_t>(img.step) * img.height, 0);

    if (whiteCol < width) {
        const size_t idx = static_cast<size_t>(whiteCol) * 3;
        img.data[idx] = 255;
        img.data[idx + 1] = 255;
        img.data[idx + 2] = 255;
    }

    return img;
}

/**
 * @brief Creates an all-black sensor_msgs::msg::Image (no ball present).
 *
 * @param width Image width in pixels.
 * @return Synthetic RGB8 image message with all pixels black.
 */
sensor_msgs::msg::Image makeBlackImage(uint32_t width) {
    sensor_msgs::msg::Image img;
    img.encoding = "rgb8";
    img.width = width;
    img.height = 1;
    img.step = width * 3;
    img.data.resize(static_cast<size_t>(img.step) * img.height, 0);
    return img;
}

} // anonymous namespace

// ─── Test fixture ────────────────────────────────────────────────────────────

/**
 * @brief Integration test fixture for BallChaserNode.
 *
 * Creates the BallChaserNode with a short lost_timeout_ms (200ms) for faster
 * tests, a publisher node for synthetic images, and subscribers for /cmd_vel
 * and /ball_chaser/state to verify outputs.
 */
class BallChaserNodeTest : public ::testing::Test { // NOLINT
  protected:
    /** @brief Set up BallChaserNode, helper node, publishers, and subscribers. */
    void SetUp() override {
        // Create BallChaserNode with parameter override for fast timeout
        rclcpp::NodeOptions options;
        options.append_parameter_override("lost_timeout_ms", 200);
        m_ballChaserNode = std::make_shared<robot_control::ball_chaser::BallChaserNode>(options);

        // Create a helper node for publishing images and subscribing to outputs
        m_helperNode = rclcpp::Node::make_shared("test_helper");

        // Publisher for synthetic camera images
        m_imagePub = m_helperNode->create_publisher<sensor_msgs::msg::Image>("/camera/rgb/image_raw", 10);

        // NOLINTBEGIN(performance-unnecessary-value-param)
        // Subscriber for /cmd_vel
        m_cmdVelSub = m_helperNode->create_subscription<geometry_msgs::msg::Twist>(
            "/cmd_vel", 10, [this](geometry_msgs::msg::Twist::SharedPtr msg) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_lastVelocity = *msg;
                m_velocityReceived = true;
                m_velocityCount++;
            }
        );

        // Subscriber for /ball_chaser/state (transient_local to receive latched)
        auto state_qos = rclcpp::QoS(10);
        state_qos.transient_local();
        m_stateSub = m_helperNode->create_subscription<std_msgs::msg::String>(
            "/ball_chaser/state", state_qos, [this](std_msgs::msg::String::SharedPtr msg) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_lastState = msg->data;
                m_stateReceived = true;
                m_stateHistory.push_back(msg->data);
            }
        );
        // NOLINTEND(performance-unnecessary-value-param)

        // Create executor and add both nodes
        m_executor = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
        m_executor->add_node(m_ballChaserNode);
        m_executor->add_node(m_helperNode);

        // Spin briefly to allow initial state publication to propagate
        spinFor(50ms);
        resetTracking();
    }

    /** @brief Tear down executor and release node resources. */
    void TearDown() override {
        m_executor->cancel();
        m_executor->remove_node(m_helperNode);
        m_executor->remove_node(m_ballChaserNode);
        m_ballChaserNode.reset();
        m_helperNode.reset();
    }

    // ─── Spin helpers ────────────────────────────────────────────────────────

    /**
     * @brief Spins the executor for a given duration.
     * @param duration How long to spin.
     */
    void spinFor(std::chrono::milliseconds duration) {
        const auto end = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < end) {
            m_executor->spin_some(5ms);
            std::this_thread::sleep_for(1ms);
        }
    }

    /**
     * @brief Spins until /ball_chaser/state matches the expected value or
     *        timeout.
     * @param expected_state The state string to wait for.
     * @param timeout Maximum time to wait.
     * @return True if the expected state was received before timeout.
     */
    bool spinUntilStateReceived(const std::string &expected_state, std::chrono::milliseconds timeout) {
        const auto end = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < end) {
            m_executor->spin_some(5ms);
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stateReceived && m_lastState == expected_state) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Spins until a /cmd_vel message is received or timeout.
     * @param timeout Maximum time to wait.
     * @return True if a velocity message was received before timeout.
     */
    bool spinUntilVelocityReceived(std::chrono::milliseconds timeout) {
        const auto end = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < end) {
            m_executor->spin_some(5ms);
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_velocityReceived) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Resets velocity and state tracking flags for a new test phase.
     */
    void resetTracking() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_velocityReceived = false;
        m_stateReceived = false;
        m_velocityCount = 0;
        m_stateHistory.clear();
    }

    // ─── Members ─────────────────────────────────────────────────────────────

    /// @brief The BallChaserNode under test.
    std::shared_ptr<robot_control::ball_chaser::BallChaserNode> m_ballChaserNode;
    /// @brief Helper node for publishing images and subscribing to outputs.
    rclcpp::Node::SharedPtr m_helperNode;
    /// @brief Single-threaded executor for spinning both nodes.
    std::shared_ptr<rclcpp::executors::SingleThreadedExecutor> m_executor;

    /// @brief Publisher for synthetic camera images.
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr m_imagePub;
    /// @brief Subscriber for /cmd_vel velocity messages.
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr m_cmdVelSub;
    /// @brief Subscriber for /ball_chaser/state messages.
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr m_stateSub;

    /// @brief Mutex protecting shared tracking state.
    std::mutex m_mutex;
    /// @brief Last received velocity message.
    geometry_msgs::msg::Twist m_lastVelocity;
    /// @brief Last received state string.
    std::string m_lastState;
    /// @brief Whether a velocity message has been received.
    bool m_velocityReceived{false};
    /// @brief Whether a state message has been received.
    bool m_stateReceived{false};
    /// @brief Count of velocity messages received.
    int m_velocityCount{0};
    /// @brief History of all state strings received.
    std::vector<std::string> m_stateHistory;
};

// ─── Test cases ──────────────────────────────────────────────────────────────

/**
 * @brief Req 7.1: Velocity published within 100ms of ball frame.
 *
 * Publishes a ball image and asserts that /cmd_vel is received within 100ms.
 */
TEST_F(BallChaserNodeTest, VelocityPublishedWithin100msOfBallFrame) {
    resetTracking();

    // Publish a ball image (white column in center zone of a 30-pixel image)
    m_imagePub->publish(makeImage(30, 15));

    // Velocity must arrive within 100ms
    EXPECT_TRUE(spinUntilVelocityReceived(100ms));
}

/**
 * @brief Req 7.2: Idle→Tracking transition on ball detection.
 *
 * Publishes a ball image and asserts /ball_chaser/state == "Tracking".
 */
TEST_F(BallChaserNodeTest, IdleToTrackingOnBallDetection) {
    resetTracking();

    // Publish a ball image
    m_imagePub->publish(makeImage(30, 15));

    // State should transition to "Tracking" within 100ms
    EXPECT_TRUE(spinUntilStateReceived("Tracking", 100ms));
}

/**
 * @brief Req 7.3: Tracking→Lost transition on ball disappearance.
 *
 * Publishes a ball image, then a black image, and asserts state == "Lost".
 */
TEST_F(BallChaserNodeTest, TrackingToLostOnBallDisappearance) {
    // First, get into Tracking state
    m_imagePub->publish(makeImage(30, 15));
    ASSERT_TRUE(spinUntilStateReceived("Tracking", 100ms));

    // Reset tracking to detect the next state change
    resetTracking();

    // Publish a black image (no ball)
    m_imagePub->publish(makeBlackImage(30));

    // State should transition to "Lost" within 100ms
    EXPECT_TRUE(spinUntilStateReceived("Lost", 100ms));
}

/**
 * @brief Req 7.4: Lost→Idle after lost_timeout with zero-velocity.
 *
 * Transitions through Tracking→Lost, waits for the 200ms timeout to expire,
 * then publishes another black frame to trigger timeout evaluation. Asserts
 * state == "Idle" and zero-velocity published.
 */
TEST_F(BallChaserNodeTest, LostToIdleAfterTimeout) {
    // Get into Tracking state
    m_imagePub->publish(makeImage(30, 15));
    ASSERT_TRUE(spinUntilStateReceived("Tracking", 100ms));

    // Transition to Lost
    resetTracking();
    m_imagePub->publish(makeBlackImage(30));
    ASSERT_TRUE(spinUntilStateReceived("Lost", 100ms));

    // Reset tracking for the Idle transition
    resetTracking();

    // Wait for lost_timeout (200ms) to expire, then publish another black
    // frame to trigger the timeout evaluation in processFrame
    std::this_thread::sleep_for(250ms);
    m_imagePub->publish(makeBlackImage(30));

    // State should transition to "Idle"
    ASSERT_TRUE(spinUntilStateReceived("Idle", 100ms));

    // Verify zero-velocity was published
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        EXPECT_TRUE(m_velocityReceived);
        EXPECT_DOUBLE_EQ(m_lastVelocity.linear.x, 0.0);
        EXPECT_DOUBLE_EQ(m_lastVelocity.angular.z, 0.0);
    }
}

/**
 * @brief Req 7.5: Lost→Tracking on ball reappearance before timeout.
 *
 * Transitions through Tracking→Lost, then quickly re-publishes a ball image
 * (well within the 200ms timeout). Asserts state == "Tracking".
 */
TEST_F(BallChaserNodeTest, LostToTrackingOnBallReappearance) {
    // Get into Tracking state
    m_imagePub->publish(makeImage(30, 15));
    ASSERT_TRUE(spinUntilStateReceived("Tracking", 100ms));

    // Transition to Lost
    resetTracking();
    m_imagePub->publish(makeBlackImage(30));
    ASSERT_TRUE(spinUntilStateReceived("Lost", 100ms));

    // Reset tracking for the Tracking transition
    resetTracking();

    // Quickly re-publish a ball image (well within the 200ms timeout)
    m_imagePub->publish(makeImage(30, 15));

    // State should transition back to "Tracking"
    EXPECT_TRUE(spinUntilStateReceived("Tracking", 100ms));
}

/**
 * @brief Req 7.6: State string values limited to "Idle", "Tracking", "Lost".
 *
 * Exercises a full state cycle and asserts all published state values are
 * one of the three valid strings.
 */
TEST_F(BallChaserNodeTest, StateValuesLimitedToValidStrings) {
    // Exercise a full state cycle: Idle → Tracking → Lost → Idle
    m_imagePub->publish(makeImage(30, 15));
    ASSERT_TRUE(spinUntilStateReceived("Tracking", 100ms));

    m_imagePub->publish(makeBlackImage(30));
    ASSERT_TRUE(spinUntilStateReceived("Lost", 100ms));

    std::this_thread::sleep_for(250ms);
    m_imagePub->publish(makeBlackImage(30));
    ASSERT_TRUE(spinUntilStateReceived("Idle", 100ms));

    // Verify all state values in history are valid
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto &state : m_stateHistory) {
        EXPECT_TRUE(state == "Idle" || state == "Tracking" || state == "Lost") << "Unexpected state value: " << state;
    }
}

/**
 * @brief Req 7.7: No velocity published in Idle with no ball for 500ms.
 *
 * Publishes only black images for 500ms and asserts no /cmd_vel is published.
 */
TEST_F(BallChaserNodeTest, NoVelocityInIdleWithNoBall) {
    resetTracking();

    // Publish only black images for 500ms observation window
    const auto end = std::chrono::steady_clock::now() + 500ms;
    while (std::chrono::steady_clock::now() < end) {
        m_imagePub->publish(makeBlackImage(30));
        m_executor->spin_some(10ms);
        std::this_thread::sleep_for(50ms);
    }

    // No velocity should have been published
    std::lock_guard<std::mutex> lock(m_mutex);
    EXPECT_FALSE(m_velocityReceived) << "Velocity was published in Idle with no ball";
    EXPECT_EQ(m_velocityCount, 0);
}

/**
 * @brief Test entry point. Initializes ROS 2 and runs all GTest cases.
 */
int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    const int result = RUN_ALL_TESTS();
    rclcpp::shutdown();
    return result;
}
