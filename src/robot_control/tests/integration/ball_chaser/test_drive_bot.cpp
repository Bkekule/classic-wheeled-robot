#include "drive_bot_node.hpp"

#include <gtest/gtest.h>
#include <custom_interfaces/srv/drive_to_target.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/rclcpp.hpp>

using DriveToTarget = custom_interfaces::srv::DriveToTarget;
using Twist = geometry_msgs::msg::Twist;

/**
 * @brief Integration tests for the DriveBot ROS 2 service node.
 *
 * Tests verify that the DriveBot service:
 * - Is properly advertised and discoverable
 * - Echoes requested velocities in the response
 * - Publishes velocity commands to /cmd_vel topic
 */
class DriveBotTest : public ::testing::Test {
  protected:
    /**
     * @brief Sets up the test environment with DriveBot server and test client.
     *
     * Initializes the DriveBot node, a test client node, creates the service requester,
     * and registers both nodes with the executor.
     */
    void SetUp() override {
        m_server = std::make_shared<robot_control::ball_chaser::DriveBot>();

        m_client = rclcpp::Node::make_shared("test_client");
        m_requester = m_client->create_client<DriveToTarget>("/ball_chaser/command_robot");

        m_executor.add_node(m_server);
        m_executor.add_node(m_client);
    }

    /**
     * @brief Tears down the test environment.
     *
     * Removes both the server and client nodes from the executor for cleanup.
     */
    void TearDown() override {
        m_executor.remove_node(m_server);
        m_executor.remove_node(m_client);
    }

    /**
     * @brief Spins the executor until a future resolves or timeout elapses.
     *
     * @tparam FutureT Type of the future (deduced).
     * @param future Future to wait for completion.
     * @param timeout Maximum time to wait (default 2 seconds).
     * @return True if the future completed, false if timeout elapsed.
     */
    template <typename FutureT>
    bool spinUntilComplete(FutureT &future, std::chrono::seconds timeout = std::chrono::seconds(2)) {
        auto deadline_ = std::chrono::steady_clock::now() + timeout;
        while (future.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready) {
            m_executor.spin_some();
            if (std::chrono::steady_clock::now() > deadline_) {
                return false;
            }
        }
        return true;
    }

    std::shared_ptr<robot_control::ball_chaser::DriveBot> m_server;       ///< DriveBot service node being tested.
    std::shared_ptr<rclcpp::Node> m_client;                               ///< Test client node.
    rclcpp::Client<DriveToTarget>::SharedPtr m_requester;                 ///< Client to call the DriveToTarget service.
    rclcpp::executors::SingleThreadedExecutor m_executor;                 ///< Executor to spin nodes during tests.
};

TEST_F(DriveBotTest, ServiceIsAdvertised) { EXPECT_TRUE(m_requester->wait_for_service(std::chrono::seconds(1))); }

TEST_F(DriveBotTest, ResponseFeedbackReflectsRequest) {
    ASSERT_TRUE(m_requester->wait_for_service(std::chrono::seconds(1)));

    auto req_ = std::make_shared<DriveToTarget::Request>();
    req_->linear_x = 0.5;
    req_->angular_z = -0.3;

    auto future_ = m_requester->async_send_request(req_);
    ASSERT_TRUE(spinUntilComplete(future_));

    auto res_ = future_.get();
    EXPECT_FALSE(res_->msg_feedback.empty());
    EXPECT_NE(res_->msg_feedback.find("0.500000"), std::string::npos);
    EXPECT_NE(res_->msg_feedback.find("-0.300000"), std::string::npos);
}

TEST_F(DriveBotTest, StopCommandPublishesZeroVelocity) {
    ASSERT_TRUE(m_requester->wait_for_service(std::chrono::seconds(1)));

    std::optional<Twist> received_;
    auto sub_ = m_client->create_subscription<Twist>(
        "/cmd_vel", 10, [&](const Twist::SharedPtr msg) { // NOLINT(performance-unnecessary-value-param)
            received_ = *msg;
        }
    );

    auto req_ = std::make_shared<DriveToTarget::Request>();
    req_->linear_x = 0.0;
    req_->angular_z = 0.0;

    auto future_ = m_requester->async_send_request(req_);
    ASSERT_TRUE(spinUntilComplete(future_));

    // Give the subscriber a chance to receive the message.
    auto deadline_ = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (!received_ && std::chrono::steady_clock::now() < deadline_) {
        m_executor.spin_some();
    }

    ASSERT_TRUE(received_.has_value());
    EXPECT_DOUBLE_EQ(received_->linear.x, 0.0);
    EXPECT_DOUBLE_EQ(received_->angular.z, 0.0);
}

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    int result_ = RUN_ALL_TESTS();
    rclcpp::shutdown();
    return result_;
}
