#include "servers/ball_chaser/drive_bot_node.hpp"

namespace robot_control::ball_chaser {

// NOLINTBEGIN(readability-function-cognitive-complexity)
DriveBot::DriveBot(const rclcpp::NodeOptions &p_options) : Node("drive_bot", p_options) {
    m_cmdVelPub = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);
    m_driveService = this->create_service<custom_interfaces::srv::DriveToTarget>(
        "/ball_chaser/command_robot", [this](auto &&PH1, auto &&PH2) {
            handleDriveRequest(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
        }
    );
    RCLCPP_INFO(this->get_logger(), "DriveBot node started, service ready on /ball_chaser/command_robot");
}
// NOLINTEND(readability-function-cognitive-complexity)

// NOLINTBEGIN(readability-function-cognitive-complexity)
void DriveBot::handleDriveRequest(
    const std::shared_ptr<custom_interfaces::srv::DriveToTarget::Request> & p_req,
    const std::shared_ptr<custom_interfaces::srv::DriveToTarget::Response> & p_res
) {
    geometry_msgs::msg::Twist l_cmd;
    l_cmd.linear.x = p_req->linear_x;
    l_cmd.angular.z = p_req->angular_z;
    m_cmdVelPub->publish(l_cmd);

    p_res->msg_feedback =
        "linear_x: " + std::to_string(p_req->linear_x) + " angular_z: " + std::to_string(p_req->angular_z);

    RCLCPP_INFO(this->get_logger(), "Published: %s", p_res->msg_feedback.c_str());
}
// NOLINTEND(readability-function-cognitive-complexity)
} // namespace robot_control::ball_chaser
