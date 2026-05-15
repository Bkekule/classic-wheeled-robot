#include "clients/ball_chaser/process_image_node.hpp"

#include <unordered_map>
#include <utility>

namespace robot_control::ball_chaser {

// NOLINTBEGIN(readability-function-cognitive-complexity)
ProcessImage::ProcessImage() : Node("process_image"), m_wasMoving(false) {
    m_velocity.linear = this->declare_parameter<double>("linear_speed", 0.5);
    m_velocity.angular = this->declare_parameter<double>("angular_speed", 0.3);

    // Per-channel lower bounds for white detection. Default 255 = pure white only.
    m_whiteThreshold.r = static_cast<uint8_t>(this->declare_parameter<int>("white_threshold_r", 255));
    m_whiteThreshold.g = static_cast<uint8_t>(this->declare_parameter<int>("white_threshold_g", 255));
    m_whiteThreshold.b = static_cast<uint8_t>(this->declare_parameter<int>("white_threshold_b", 255));

    m_driveClient = this->create_client<custom_interfaces::srv::DriveToTarget>("/ball_chaser/command_robot");

    m_imageSub = this->create_subscription<sensor_msgs::msg::Image>(
        "/camera/rgb/image_raw", 10,
        [this](sensor_msgs::msg::Image::SharedPtr p_msg) { imageCallback(std::move(p_msg)); }
    );

    RCLCPP_INFO(this->get_logger(), "ProcessImage node started");
}

// NOLINTBEGIN(performance-unnecessary-value-param)
void ProcessImage::imageCallback(const sensor_msgs::msg::Image::SharedPtr p_msg) {
    using Handler = void (ProcessImage::*)(const sensor_msgs::msg::Image::SharedPtr &);

    static const std::unordered_map<std::string, Handler> k_handlers = {
        {"rgb8", &ProcessImage::processRgb8},
    };

    auto handler = k_handlers.find(p_msg->encoding);
    if (handler == k_handlers.end()) {
        RCLCPP_WARN_ONCE(
            this->get_logger(), "Unsupported encoding '%s' — no handler registered", p_msg->encoding.c_str()
        );
        return;
    }
    (this->*(handler->second))(p_msg);
}
// NOLINTEND(performance-unnecessary-value-param)
// NOLINTEND(readability-function-cognitive-complexity)

void ProcessImage::processRgb8(const sensor_msgs::msg::Image::SharedPtr &p_msg) {
    const uint32_t l_width = p_msg->width;
    const uint32_t l_height = p_msg->height;
    const uint32_t l_step = p_msg->step;
    constexpr uint8_t l_bpp = 3;

    int l_ballXSum = 0;
    int l_ballCount = 0;

    for (uint32_t l_row = 0; l_row < l_height; ++l_row) {
        for (uint32_t l_col = 0; l_col < l_width; ++l_col) {
            const size_t l_idx = (l_row * l_step) + (l_col * l_bpp);
            const uint8_t l_red = p_msg->data[l_idx];
            const uint8_t l_green = p_msg->data[l_idx + 1];
            const uint8_t l_blue = p_msg->data[l_idx + 2];

            if (l_red >= m_whiteThreshold.r && l_green >= m_whiteThreshold.g && l_blue >= m_whiteThreshold.b) {
                l_ballXSum += static_cast<int>(l_col);
                ++l_ballCount;
            }
        }
    }

    if (l_ballCount == 0) {
        if (m_wasMoving) {
            driveRobot({0.0, 0.0});
            m_wasMoving = false;
        }
        return;
    }

    const int l_ballCol = l_ballXSum / l_ballCount;
    const int l_leftBound = static_cast<int>(l_width) / 3;
    const int l_rightBound = static_cast<int>(l_width) * 2 / 3;

    if (l_ballCol < l_leftBound) {
        driveRobot({0.0, m_velocity.angular});
    } else if (l_ballCol > l_rightBound) {
        driveRobot({0.0, -m_velocity.angular});
    } else {
        driveRobot({m_velocity.linear, 0.0});
    }

    m_wasMoving = true;
}

// NOLINTBEGIN(readability-function-cognitive-complexity)
void ProcessImage::driveRobot(const VelocityParams &p_velocity) {
    if (!m_driveClient->wait_for_service(std::chrono::milliseconds(100))) {
        RCLCPP_WARN(this->get_logger(), "DriveToTarget service not available");
        return;
    }

    auto l_request = std::make_shared<custom_interfaces::srv::DriveToTarget::Request>();
    l_request->linear_x = p_velocity.linear;
    l_request->angular_z = p_velocity.angular;

    m_driveClient->async_send_request(
        l_request, [this](
                       rclcpp::Client<custom_interfaces::srv::DriveToTarget>::SharedFuture
                           l_future // NOLINT(performance-unnecessary-value-param)
                   ) { RCLCPP_DEBUG(this->get_logger(), "Drive response: %s", l_future.get()->msg_feedback.c_str()); }
    );
}
// NOLINTEND(readability-function-cognitive-complexity)

} // namespace robot_control::ball_chaser
