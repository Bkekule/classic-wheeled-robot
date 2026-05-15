#include "drive_bot_node.hpp"

#include <rclcpp/rclcpp.hpp>

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<robot_control::ball_chaser::DriveBot>());
    rclcpp::shutdown();
    return 0;
}
