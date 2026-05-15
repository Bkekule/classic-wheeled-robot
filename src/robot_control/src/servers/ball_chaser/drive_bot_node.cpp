#include "servers/ball_chaser/drive_bot_node.hpp"

#include <utility>

namespace robot_control::ball_chaser {

// NOLINTBEGIN(readability-function-cognitive-complexity)
DriveBot::DriveBot() : Node("drive_bot") {
    m_cmdVelPub = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);
    m_driveService = this->create_service<custom_interfaces::srv::DriveToTarget>(
        "/ball_chaser/command_robot", [this](
                                          custom_interfaces::srv::DriveToTarget::Request::SharedPtr p_req,
                                          custom_interfaces::srv::DriveToTarget::Response::SharedPtr p_res
                                      ) { handleDriveRequest(std::move(p_req), std::move(p_res)); }
    );
    RCLCPP_INFO(this->get_logger(), "DriveBot node started, service ready on /ball_chaser/command_robot");
}

// NOLINTBEGIN(performance-unnecessary-value-param)
void DriveBot::handleDriveRequest(
    const custom_interfaces::srv::DriveToTarget::Request::SharedPtr p_req,
    const custom_interfaces::srv::DriveToTarget::Response::SharedPtr p_res
) {
    geometry_msgs::msg::Twist l_cmd;
    l_cmd.linear.x = p_req->linear_x;
    l_cmd.angular.z = p_req->angular_z;
    m_cmdVelPub->publish(l_cmd);

    p_res->msg_feedback =
        "linear_x: " + std::to_string(p_req->linear_x) + " angular_z: " + std::to_string(p_req->angular_z);

    RCLCPP_INFO(this->get_logger(), "Published: %s", p_res->msg_feedback.c_str());
}
// NOLINTEND(performance-unnecessary-value-param)
// NOLINTEND(readability-function-cognitive-complexity)
} // namespace robot_control::ball_chaser
