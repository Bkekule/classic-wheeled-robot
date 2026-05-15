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
        "/camera/rgb/image_raw", 10, [this](sensor_msgs::msg::Image::SharedPtr msg) { imageCallback(std::move(msg)); }
    );

    RCLCPP_INFO(this->get_logger(), "ProcessImage node started");
}

// NOLINTBEGIN(performance-unnecessary-value-param)
void ProcessImage::imageCallback(const sensor_msgs::msg::Image::SharedPtr msg) {
    using Handler = void (ProcessImage::*)(const sensor_msgs::msg::Image::SharedPtr &);

    static const std::unordered_map<std::string, Handler> k_handlers = {
        {"rgb8", &ProcessImage::processRgb8},
    };

    auto handler = k_handlers.find(msg->encoding);
    if (handler == k_handlers.end()) {
        RCLCPP_WARN_ONCE(
            this->get_logger(), "Unsupported encoding '%s' — no handler registered", msg->encoding.c_str()
        );
        return;
    }
    (this->*(handler->second))(msg);
}
// NOLINTEND(performance-unnecessary-value-param)
// NOLINTEND(readability-function-cognitive-complexity)

void ProcessImage::processRgb8(const sensor_msgs::msg::Image::SharedPtr &msg) {
    const uint32_t width_ = msg->width;
    const uint32_t height_ = msg->height;
    const uint32_t step_ = msg->step;
    constexpr uint8_t bpp_ = 3;

    int ballXSum_ = 0;
    int ballCount_ = 0;

    for (uint32_t row_ = 0; row_ < height_; ++row_) {
        for (uint32_t col_ = 0; col_ < width_; ++col_) {
            const size_t idx_ = (row_ * step_) + (col_ * bpp_);
            const uint8_t red_ = msg->data[idx_];
            const uint8_t green_ = msg->data[idx_ + 1];
            const uint8_t blue_ = msg->data[idx_ + 2];

            if (red_ >= m_whiteThreshold.r && green_ >= m_whiteThreshold.g && blue_ >= m_whiteThreshold.b) {
                ballXSum_ += static_cast<int>(col_);
                ++ballCount_;
            }
        }
    }

    if (ballCount_ == 0) {
        if (m_wasMoving) {
            driveRobot({0.0, 0.0});
            m_wasMoving = false;
        }
        return;
    }

    const int ballCol_ = ballXSum_ / ballCount_;
    const int leftBound_ = static_cast<int>(width_) / 3;
    const int rightBound_ = static_cast<int>(width_) * 2 / 3;

    if (ballCol_ < leftBound_) {
        driveRobot({0.0, m_velocity.angular});
    } else if (ballCol_ > rightBound_) {
        driveRobot({0.0, -m_velocity.angular});
    } else {
        driveRobot({m_velocity.linear, 0.0});
    }

    m_wasMoving = true;
}

// NOLINTBEGIN(readability-function-cognitive-complexity)
void ProcessImage::driveRobot(const VelocityParams &velocity) {
    if (!m_driveClient->wait_for_service(std::chrono::milliseconds(100))) {
        RCLCPP_WARN(this->get_logger(), "DriveToTarget service not available");
        return;
    }

    auto request_ = std::make_shared<custom_interfaces::srv::DriveToTarget::Request>();
    request_->linear_x = velocity.linear;
    request_->angular_z = velocity.angular;

    m_driveClient->async_send_request(
        request_, [this](
                      rclcpp::Client<custom_interfaces::srv::DriveToTarget>::SharedFuture
                          future_ // NOLINT(performance-unnecessary-value-param)
                  ) { RCLCPP_DEBUG(this->get_logger(), "Drive response: %s", future_.get()->msg_feedback.c_str()); }
    );
}
// NOLINTEND(readability-function-cognitive-complexity)

} // namespace robot_control::ball_chaser
