#pragma once

#include <custom_interfaces/srv/drive_to_target.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/rclcpp.hpp>

namespace robot_control::ball_chaser {

class DriveBot : public rclcpp::Node {
  public:
    explicit DriveBot(const rclcpp::NodeOptions &p_options = rclcpp::NodeOptions());

  private:
    void handleDriveRequest(
        const std::shared_ptr<custom_interfaces::srv::DriveToTarget::Request> &p_req,
        const std::shared_ptr<custom_interfaces::srv::DriveToTarget::Response> &p_res
    );

    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr m_cmdVelPub;
    rclcpp::Service<custom_interfaces::srv::DriveToTarget>::SharedPtr m_driveService;
};

} // namespace robot_control::ball_chaser
