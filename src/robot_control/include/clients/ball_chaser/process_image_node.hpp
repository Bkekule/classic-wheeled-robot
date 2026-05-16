#pragma once

#include "image_utils.hpp"

#include <custom_interfaces/srv/drive_to_target.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>

namespace robot_control::ball_chaser {

/**
 * @brief ROS 2 client node that processes camera images to detect and chase a white ball.
 *
 * Subscribes to camera images, detects the white ball using configurable RGB thresholds,
 * and commands the robot to drive toward it. Stops the robot when the ball is not detected.
 * Linear and angular speeds are configurable via ROS parameters.
 */
class ProcessImage : public rclcpp::Node {
  public:
    /**
     * @brief Constructs and initializes the ProcessImage node.
     *
     * Initializes camera subscription, drive client, and loads configuration parameters
     * for ball detection thresholds and velocity limits.
     */
    ProcessImage();

  private:
    /**
     * @brief Linear and angular velocity components.
     */
    struct VelocityParams {
        double linear;  ///< Linear velocity in m/s.
        double angular; ///< Angular velocity in rad/s.
    };

    /**
     * @brief Routes incoming images to the appropriate handler based on encoding.
     *
     * @param msg Camera image message.
     */
    void imageCallback(sensor_msgs::msg::Image::SharedPtr msg);

    /**
     * @brief Processes RGB8 encoded images to detect the ball and generate drive commands.
     *
     * @param msg RGB8 encoded camera image.
     */
    void processRgb8(const sensor_msgs::msg::Image::SharedPtr &msg);

    /**
     * @brief Sends velocity commands to the robot via the DriveToTarget service.
     *
     * @param velocity Desired linear and angular velocities.
     */
    void driveRobot(const VelocityParams &velocity);

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr m_imageSub;
    rclcpp::Client<custom_interfaces::srv::DriveToTarget>::SharedPtr m_driveClient;

    VelocityParams m_velocity;
    RgbThreshold m_whiteBallThreshold;

    bool m_wasMoving;
};

} // namespace robot_control::ball_chaser
