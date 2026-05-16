#pragma once

#include <custom_interfaces/srv/drive_to_target.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/rclcpp.hpp>

namespace robot_control::ball_chaser {

/**
 * @brief ROS 2 service node that relays velocity commands to the robot.
 *
 * Exposes the /ball_chaser/command_robot service which accepts linear and angular
 * velocity values and publishes them to /cmd_vel for the robot to execute.
 */
class DriveBot : public rclcpp::Node {
  public:
    /**
     * @brief Constructs and initializes the DriveBot node.
     *
     * Sets up the /ball_chaser/command_robot service and /cmd_vel publisher.
     */
    DriveBot();

  private:
    /**
     * @brief Handles incoming drive requests and publishes velocity commands.
     *
     * @param req Service request containing linear_x and angular_z velocities.
     * @param res Service response with feedback on published velocities.
     */
    void handleDriveRequest(
        custom_interfaces::srv::DriveToTarget::Request::SharedPtr req,
        custom_interfaces::srv::DriveToTarget::Response::SharedPtr res
    );

    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr m_cmdVelPub;
    rclcpp::Service<custom_interfaces::srv::DriveToTarget>::SharedPtr m_driveService;
};

} // namespace robot_control::ball_chaser
