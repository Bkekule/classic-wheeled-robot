/**
 * @file ball_chaser_node.cpp
 * @brief Node construction, parameter declaration, and ROS interface wiring.
 */

#include "ball_chaser_node.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

namespace robot_control::ball_chaser {

namespace {
constexpr double kDefaultLeftZoneRatio = 0.33;
constexpr double kDefaultRightZoneRatio = 0.67;
constexpr uint16_t kMinLostTimeoutMs = 100;
constexpr uint16_t kMaxLostTimeoutMs = 10000;
} // namespace

// NOLINTBEGIN(readability-function-cognitive-complexity)
BallChaserNode::BallChaserNode() : BallChaserNode(rclcpp::NodeOptions()) {}

BallChaserNode::BallChaserNode(const rclcpp::NodeOptions &options) : Node("ball_chaser", options) {
    // ─── Declare and retrieve parameters ─────────────────────────────────────

    m_linearSpeed = this->declare_parameter<double>("linear_speed", 0.5);
    m_angularSpeed = this->declare_parameter<double>("angular_speed", 0.3);

    m_whiteThreshold.r = static_cast<uint8_t>(this->declare_parameter<int64_t>("white_threshold_r", 255));
    m_whiteThreshold.g = static_cast<uint8_t>(this->declare_parameter<int64_t>("white_threshold_g", 255));
    m_whiteThreshold.b = static_cast<uint8_t>(this->declare_parameter<int64_t>("white_threshold_b", 255));

    m_zoneRatios.left = this->declare_parameter<double>("left_zone_ratio", kDefaultLeftZoneRatio);
    m_zoneRatios.right = this->declare_parameter<double>("right_zone_ratio", kDefaultRightZoneRatio);

    m_lostTimeoutMs = static_cast<uint16_t>(this->declare_parameter<int64_t>("lost_timeout_ms", 500));

    // ─── Validate zone ratios ────────────────────────────────────────────────

    if (m_zoneRatios.left >= m_zoneRatios.right || m_zoneRatios.left <= 0.0 || m_zoneRatios.left >= 1.0 ||
        m_zoneRatios.right <= 0.0 || m_zoneRatios.right >= 1.0) {
        RCLCPP_WARN(
            this->get_logger(), "Invalid zone ratios (left=%.3f, right=%.3f). Using defaults (%.2f, %.2f).",
            m_zoneRatios.left, m_zoneRatios.right, kDefaultLeftZoneRatio, kDefaultRightZoneRatio
        );
        m_zoneRatios.left = kDefaultLeftZoneRatio;
        m_zoneRatios.right = kDefaultRightZoneRatio;
    }

    // ─── Validate lost_timeout_ms ────────────────────────────────────────────

    if (m_lostTimeoutMs < kMinLostTimeoutMs || m_lostTimeoutMs > kMaxLostTimeoutMs) {
        RCLCPP_WARN(
            this->get_logger(), "lost_timeout_ms=%u outside valid range [%u, %u]. Clamping.", m_lostTimeoutMs,
            kMinLostTimeoutMs, kMaxLostTimeoutMs
        );
        m_lostTimeoutMs = std::clamp(m_lostTimeoutMs, kMinLostTimeoutMs, kMaxLostTimeoutMs);
    }

    // ─── Publishers ──────────────────────────────────────────────────────────

    m_cmdVelPub = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);

    auto state_qos = rclcpp::QoS(10);
    state_qos.transient_local();
    m_statePub = this->create_publisher<std_msgs::msg::String>("/ball_chaser/state", state_qos);

    m_diagPub = this->create_publisher<diagnostic_msgs::msg::DiagnosticArray>("/diagnostics", 10);

    // ─── Subscriber ──────────────────────────────────────────────────────────

    m_imageSub = this->create_subscription<sensor_msgs::msg::Image>(
        "/camera/rgb/image_raw", 10, [this](sensor_msgs::msg::Image::SharedPtr msg) { imageCallback(std::move(msg)); }
    );

    // ─── Service ─────────────────────────────────────────────────────────────
    // NOLINTBEGIN(performance-unnecessary-value-param)
    m_commandService = this->create_service<custom_interfaces::srv::BallChaserCommand>(
        "/ball_chaser/command", [this](
                                    const custom_interfaces::srv::BallChaserCommand::Request::SharedPtr req,
                                    const custom_interfaces::srv::BallChaserCommand::Response::SharedPtr res
                                ) { handleCommand(req, res); }
    );
    // NOLINTEND(performance-unnecessary-value-param)
    // ─── Diagnostics timer (1 Hz) ───────────────────────────────────────────

    m_diagTimer = this->create_wall_timer(std::chrono::seconds(1), [this]() { publishDiagnostics(); });

    // ─── Publish initial state ───────────────────────────────────────────────

    publishState(BehaviorState::Idle);

    RCLCPP_INFO(this->get_logger(), "BallChaserNode started in Idle state");
}
// NOLINTEND(readability-function-cognitive-complexity)

} // namespace robot_control::ball_chaser
