#include "servers/ball_chaser/drive_bot_node.hpp"

#include <gtest/gtest.h>
#include <custom_interfaces/srv/drive_to_target.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/rclcpp.hpp>

using DriveToTarget = custom_interfaces::srv::DriveToTarget;
using Twist = geometry_msgs::msg::Twist;

class DriveBotTest : public ::testing::Test {
  protected:
    void SetUp() override {
        m_server = std::make_shared<robot_control::ball_chaser::DriveBot>();

        m_client = rclcpp::Node::make_shared("test_client");
        m_requester = m_client->create_client<DriveToTarget>("/ball_chaser/command_robot");

        m_executor.add_node(m_server);
        m_executor.add_node(m_client);
    }

    void TearDown() override {
        m_executor.remove_node(m_server);
        m_executor.remove_node(m_client);
    }

    // Spin until the future resolves or timeout elapses.
    template <typename FutureT>
    bool spinUntilComplete(FutureT &p_future, std::chrono::seconds p_timeout = std::chrono::seconds(2)) {
        auto l_deadline = std::chrono::steady_clock::now() + p_timeout;
        while (p_future.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready) {
            m_executor.spin_some();
            if (std::chrono::steady_clock::now() > l_deadline) {
                return false;
            }
        }
        return true;
    }

    std::shared_ptr<robot_control::ball_chaser::DriveBot> m_server;
    std::shared_ptr<rclcpp::Node> m_client;
    rclcpp::Client<DriveToTarget>::SharedPtr m_requester;
    rclcpp::executors::SingleThreadedExecutor m_executor;
};

TEST_F(DriveBotTest, ServiceIsAdvertised) { EXPECT_TRUE(m_requester->wait_for_service(std::chrono::seconds(1))); }

TEST_F(DriveBotTest, ResponseFeedbackReflectsRequest) {
    ASSERT_TRUE(m_requester->wait_for_service(std::chrono::seconds(1)));

    auto l_req = std::make_shared<DriveToTarget::Request>();
    l_req->linear_x = 0.5;
    l_req->angular_z = -0.3;

    auto l_future = m_requester->async_send_request(l_req);
    ASSERT_TRUE(spinUntilComplete(l_future));

    auto l_res = l_future.get();
    EXPECT_FALSE(l_res->msg_feedback.empty());
    EXPECT_NE(l_res->msg_feedback.find("0.500000"), std::string::npos);
    EXPECT_NE(l_res->msg_feedback.find("-0.300000"), std::string::npos);
}

TEST_F(DriveBotTest, StopCommandPublishesZeroVelocity) {
    ASSERT_TRUE(m_requester->wait_for_service(std::chrono::seconds(1)));

    std::optional<Twist> l_received;
    auto l_sub = m_client->create_subscription<Twist>(
        "/cmd_vel", 10, [&](const Twist::SharedPtr p_msg) { // NOLINT(performance-unnecessary-value-param)
            l_received = *p_msg;
        }
    );

    auto l_req = std::make_shared<DriveToTarget::Request>();
    l_req->linear_x = 0.0;
    l_req->angular_z = 0.0;

    auto l_future = m_requester->async_send_request(l_req);
    ASSERT_TRUE(spinUntilComplete(l_future));

    // Give the subscriber a chance to receive the message.
    auto l_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (!l_received && std::chrono::steady_clock::now() < l_deadline) {
        m_executor.spin_some();
    }

    ASSERT_TRUE(l_received.has_value());
    EXPECT_DOUBLE_EQ(l_received->linear.x, 0.0);
    EXPECT_DOUBLE_EQ(l_received->angular.z, 0.0);
}

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    int l_result = RUN_ALL_TESTS();
    rclcpp::shutdown();
    return l_result;
}
