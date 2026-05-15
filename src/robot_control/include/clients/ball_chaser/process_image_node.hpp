#pragma once

#include <custom_interfaces/srv/drive_to_target.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>

namespace robot_control::ball_chaser {

class ProcessImage : public rclcpp::Node {
  public:
    ProcessImage();

  private:
    struct VelocityParams {
        double linear;
        double angular;
    };

    struct WhiteThreshold {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    void imageCallback(sensor_msgs::msg::Image::SharedPtr msg);
    void processRgb8(const sensor_msgs::msg::Image::SharedPtr &msg);
    void driveRobot(const VelocityParams &velocity);

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr m_imageSub;
    rclcpp::Client<custom_interfaces::srv::DriveToTarget>::SharedPtr m_driveClient;

    VelocityParams m_velocity;
    WhiteThreshold m_whiteThreshold;

    bool m_wasMoving;
};

} // namespace robot_control::ball_chaser
