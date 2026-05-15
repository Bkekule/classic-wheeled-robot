#pragma once

#include <custom_interfaces/srv/drive_to_target.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/rclcpp.hpp>

namespace robot_control::ball_chaser {

class DriveBot : public rclcpp::Node {
  public:
    DriveBot();

  private:
    void handleDriveRequest(
        custom_interfaces::srv::DriveToTarget::Request::SharedPtr req,
        custom_interfaces::srv::DriveToTarget::Response::SharedPtr res
    );

    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr m_cmdVelPub;
    rclcpp::Service<custom_interfaces::srv::DriveToTarget>::SharedPtr m_driveService;
};

} // namespace robot_control::ball_chaser
