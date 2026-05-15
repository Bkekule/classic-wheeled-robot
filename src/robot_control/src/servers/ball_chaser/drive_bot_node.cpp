#include "servers/ball_chaser/drive_bot_node.hpp"

#include <utility>

namespace robot_control::ball_chaser {

// NOLINTBEGIN(readability-function-cognitive-complexity)
DriveBot::DriveBot() : Node("drive_bot") {
    m_cmdVelPub = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);
    m_driveService = this->create_service<custom_interfaces::srv::DriveToTarget>(
        "/ball_chaser/command_robot", [this](
                                          custom_interfaces::srv::DriveToTarget::Request::SharedPtr req,
                                          custom_interfaces::srv::DriveToTarget::Response::SharedPtr res
                                      ) { handleDriveRequest(std::move(req), std::move(res)); }
    );
    RCLCPP_INFO(this->get_logger(), "DriveBot node started, service ready on /ball_chaser/command_robot");
}

// NOLINTBEGIN(performance-unnecessary-value-param)
void DriveBot::handleDriveRequest(
    const custom_interfaces::srv::DriveToTarget::Request::SharedPtr req,
    const custom_interfaces::srv::DriveToTarget::Response::SharedPtr res
) {
    geometry_msgs::msg::Twist cmd_;
    cmd_.linear.x = req->linear_x;
    cmd_.angular.z = req->angular_z;
    m_cmdVelPub->publish(cmd_);

    res->msg_feedback = "linear_x: " + std::to_string(req->linear_x) + " angular_z: " + std::to_string(req->angular_z);

    RCLCPP_INFO(this->get_logger(), "Published: %s", res->msg_feedback.c_str());
}
// NOLINTEND(performance-unnecessary-value-param)
// NOLINTEND(readability-function-cognitive-complexity)
} // namespace robot_control::ball_chaser
