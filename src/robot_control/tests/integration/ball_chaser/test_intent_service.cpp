/**
 * @file test_intent_service.cpp
 * @brief Integration tests for the /ball_chaser/command intent service.
 *
 * Validates Requirements: 7.8, 6.3, 6.4, 6.5, 6.6
 */

#include "ball_chaser_node.hpp"

#include <chrono>
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <custom_interfaces/srv/ball_chaser_command.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

using namespace std::chrono_literals;
using BallChaserCommand = custom_interfaces::srv::BallChaserCommand; ///< Shortened BallChasserCommand namespace

/**
 * @brief Integration test fixture for the /ball_chaser/command intent service.
 *
 * Creates a BallChaserNode and a client node to send service requests,
 * subscribing to /cmd_vel and /ball_chaser/state to verify outputs.
 */
class IntentServiceTest : public ::testing::Test {
  protected:
    /** @brief Set up ROS 2 context, nodes, service client, and subscribers. */
    void SetUp() override {
        rclcpp::init(0, nullptr);

        // Create the BallChaserNode under test
        ball_chaser_node_ = std::make_shared<robot_control::ball_chaser::BallChaserNode>();

        // Create a client node for sending service requests and subscribing
        client_node_ = std::make_shared<rclcpp::Node>("test_intent_client");

        // Create service client
        service_client_ = client_node_->create_client<BallChaserCommand>("/ball_chaser/command");

        // NOLINTBEGIN(performance-unnecessary-value-param)
        // Subscribe to /cmd_vel
        cmd_vel_received_ = false;
        last_cmd_vel_ = geometry_msgs::msg::Twist();
        cmd_vel_sub_ = client_node_->create_subscription<geometry_msgs::msg::Twist>(
            "/cmd_vel", 10, [this](geometry_msgs::msg::Twist::SharedPtr msg) {
                last_cmd_vel_ = *msg;
                cmd_vel_received_ = true;
            }
        );

        // Subscribe to /ball_chaser/state
        state_received_ = false;
        last_state_ = "";
        auto state_qos = rclcpp::QoS(10);
        state_qos.transient_local();
        state_sub_ = client_node_->create_subscription<std_msgs::msg::String>(
            "/ball_chaser/state", state_qos, [this](std_msgs::msg::String::SharedPtr msg) {
                last_state_ = msg->data;
                state_received_ = true;
            }
        );
        // NOLINTEND(performance-unnecessary-value-param)

        // Set up executor
        executor_ = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
        executor_->add_node(ball_chaser_node_);
        executor_->add_node(client_node_);

        // Wait for service to be available
        ASSERT_TRUE(service_client_->wait_for_service(5s)) << "Service /ball_chaser/command not available";

        // Spin briefly to process initial state publication
        spinFor(100ms);
    }

    /** @brief Tear down ROS 2 context and release all resources. */
    void TearDown() override {
        executor_->cancel();
        executor_->remove_node(client_node_);
        executor_->remove_node(ball_chaser_node_);
        service_client_.reset();
        cmd_vel_sub_.reset();
        state_sub_.reset();
        client_node_.reset();
        ball_chaser_node_.reset();
        executor_.reset();
        rclcpp::shutdown();
    }

    /**
     * @brief Spin the executor for a given duration.
     * @param duration How long to spin.
     */
    void spinFor(std::chrono::milliseconds duration) {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < duration) {
            executor_->spin_some(10ms);
        }
    }

    /**
     * @brief Send a command and wait for the response.
     * @param command The command string to send to the service.
     * @return The service response, or nullptr on timeout.
     */
    BallChaserCommand::Response::SharedPtr sendCommand(const std::string &command) {
        auto request = std::make_shared<BallChaserCommand::Request>();
        request->command = command;

        auto future = service_client_->async_send_request(request);

        // Spin until we get the response or timeout
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < 2s) {
            executor_->spin_some(10ms);
            if (future.wait_for(0ms) == std::future_status::ready) {
                return future.get();
            }
        }
        return nullptr;
    }

    /// @brief The BallChaserNode under test.
    std::shared_ptr<robot_control::ball_chaser::BallChaserNode> ball_chaser_node_;
    /// @brief Client node for sending service requests and subscribing.
    std::shared_ptr<rclcpp::Node> client_node_;

    /// @brief Single-threaded executor for spinning both nodes.
    std::shared_ptr<rclcpp::executors::SingleThreadedExecutor> executor_;

    /// @brief Service client for /ball_chaser/command.
    rclcpp::Client<BallChaserCommand>::SharedPtr service_client_;

    /// @brief Subscriber for /cmd_vel velocity messages.
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
    /// @brief Subscriber for /ball_chaser/state messages.
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr state_sub_;

    /// @brief Whether a /cmd_vel message has been received.
    bool cmd_vel_received_;
    /// @brief Last received velocity message.
    geometry_msgs::msg::Twist last_cmd_vel_;
    /// @brief Whether a /ball_chaser/state message has been received.
    bool state_received_;
    /// @brief Last received state string.
    std::string last_state_;
};

/**
 * @brief Test that an unrecognized command returns accepted=false and no
 *        velocity is published within 200ms.
 *
 * Validates: Requirement 7.8
 */
TEST_F(IntentServiceTest, UnrecognizedCommandRejected) {
    // Reset cmd_vel tracking
    cmd_vel_received_ = false;

    auto response = sendCommand("INVALID");
    ASSERT_NE(response, nullptr) << "Service call timed out";
    EXPECT_FALSE(response->accepted);

    // Wait 200ms and verify no velocity was published
    cmd_vel_received_ = false;
    spinFor(200ms);
    EXPECT_FALSE(cmd_vel_received_) << "Velocity should not be published for unrecognized command";
}

/**
 * @brief Test that FORWARD command produces correct velocity and Tracking state.
 *
 * Validates: Requirement 6.3
 */
TEST_F(IntentServiceTest, ForwardCommandPublishesVelocity) {
    cmd_vel_received_ = false;

    auto response = sendCommand("FORWARD");
    ASSERT_NE(response, nullptr) << "Service call timed out";
    EXPECT_TRUE(response->accepted);

    // Spin to receive published messages
    spinFor(100ms);

    ASSERT_TRUE(cmd_vel_received_) << "Expected /cmd_vel to be published";
    EXPECT_GT(last_cmd_vel_.linear.x, 0.0) << "FORWARD should have positive linear.x";
    EXPECT_DOUBLE_EQ(last_cmd_vel_.angular.z, 0.0) << "FORWARD should have zero angular.z";

    // Verify state transitioned to Tracking
    EXPECT_EQ(last_state_, "Tracking");
}

/**
 * @brief Test that TURN_LEFT command produces correct velocity and Tracking state.
 *
 * Validates: Requirement 6.4
 */
TEST_F(IntentServiceTest, TurnLeftCommandPublishesVelocity) {
    cmd_vel_received_ = false;

    auto response = sendCommand("TURN_LEFT");
    ASSERT_NE(response, nullptr) << "Service call timed out";
    EXPECT_TRUE(response->accepted);

    // Spin to receive published messages
    spinFor(100ms);

    ASSERT_TRUE(cmd_vel_received_) << "Expected /cmd_vel to be published";
    EXPECT_DOUBLE_EQ(last_cmd_vel_.linear.x, 0.0) << "TURN_LEFT should have zero linear.x";
    EXPECT_GT(last_cmd_vel_.angular.z, 0.0) << "TURN_LEFT should have positive angular.z";

    // Verify state transitioned to Tracking
    EXPECT_EQ(last_state_, "Tracking");
}

/**
 * @brief Test that TURN_RIGHT command produces correct velocity and Tracking state.
 *
 * Validates: Requirement 6.5
 */
TEST_F(IntentServiceTest, TurnRightCommandPublishesVelocity) {
    cmd_vel_received_ = false;

    auto response = sendCommand("TURN_RIGHT");
    ASSERT_NE(response, nullptr) << "Service call timed out";
    EXPECT_TRUE(response->accepted);

    // Spin to receive published messages
    spinFor(100ms);

    ASSERT_TRUE(cmd_vel_received_) << "Expected /cmd_vel to be published";
    EXPECT_DOUBLE_EQ(last_cmd_vel_.linear.x, 0.0) << "TURN_RIGHT should have zero linear.x";
    EXPECT_LT(last_cmd_vel_.angular.z, 0.0) << "TURN_RIGHT should have negative angular.z";

    // Verify state transitioned to Tracking
    EXPECT_EQ(last_state_, "Tracking");
}

/**
 * @brief Test that STOP command produces zero-velocity and transitions to Idle.
 *
 * First sends FORWARD to get into Tracking state, then sends STOP.
 *
 * Validates: Requirement 6.6
 */
TEST_F(IntentServiceTest, StopCommandPublishesZeroVelocity) {
    // First, move into Tracking state
    auto fwd_response = sendCommand("FORWARD");
    ASSERT_NE(fwd_response, nullptr) << "FORWARD service call timed out";
    ASSERT_TRUE(fwd_response->accepted);
    spinFor(50ms);

    // Now send STOP
    cmd_vel_received_ = false;
    auto response = sendCommand("STOP");
    ASSERT_NE(response, nullptr) << "STOP service call timed out";
    EXPECT_TRUE(response->accepted);

    // Spin to receive published messages
    spinFor(100ms);

    ASSERT_TRUE(cmd_vel_received_) << "Expected /cmd_vel to be published for STOP";
    EXPECT_DOUBLE_EQ(last_cmd_vel_.linear.x, 0.0) << "STOP should have zero linear.x";
    EXPECT_DOUBLE_EQ(last_cmd_vel_.angular.z, 0.0) << "STOP should have zero angular.z";

    // Verify state transitioned to Idle
    EXPECT_EQ(last_state_, "Idle");
}

/**
 * @brief Test that command matching is case-sensitive.
 *
 * Sending "forward" (lowercase) should be rejected.
 *
 * Validates: Requirement 6.8 (case-sensitive matching)
 */
TEST_F(IntentServiceTest, CaseSensitiveMatching) {
    cmd_vel_received_ = false;

    auto response = sendCommand("forward");
    ASSERT_NE(response, nullptr) << "Service call timed out";
    EXPECT_FALSE(response->accepted);

    // Wait 200ms and verify no velocity was published
    cmd_vel_received_ = false;
    spinFor(200ms);
    EXPECT_FALSE(cmd_vel_received_) << "Velocity should not be published for lowercase command";
}

/**
 * @brief Test entry point. Runs all GTest cases for the intent service.
 */
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
