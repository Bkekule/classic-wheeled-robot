#pragma once

#include "image_utils.hpp"
#include "state_machine.hpp"

#include <chrono>
#include <cstdint>

#include <custom_interfaces/srv/ball_chaser_command.hpp>
#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <std_msgs/msg/string.hpp>

namespace robot_control::ball_chaser {

/**
 * @brief Diagnostic telemetry data accumulated between diagnostic publishes.
 */
struct DiagnosticData {
    uint64_t frames_processed{0};                           ///< Cumulative frames processed since node start.
    BehaviorState current_state{BehaviorState::Idle};       ///< Current behavior state.
    uint32_t detections_since_last_report{0};               ///< Detections since last diagnostic publish.
    BallRegion last_detection_region{BallRegion::NotFound}; ///< Most recent detection result.
};

/**
 * @brief Unified ball chaser ROS 2 node.
 *
 * Subscribes to camera images, detects the white ball using configurable RGB
 * thresholds, manages a finite state machine (Idle → Tracking → Lost → Idle),
 * and publishes velocity commands directly to /cmd_vel. Provides observability
 * via /ball_chaser/state and /diagnostics topics, and an intent-based service
 * interface at /ball_chaser/command for external control.
 */
class BallChaserNode : public rclcpp::Node {
  public:
    /**
     * @brief Constructs and initializes the BallChaserNode with default options.
     *
     * Declares all ROS 2 parameters, creates publishers, subscriber, service,
     * and diagnostics timer. Publishes initial "Idle" state on construction.
     */
    BallChaserNode();

    /**
     * @brief Constructs and initializes the BallChaserNode with custom options.
     *
     * Allows parameter overrides for testing and launch-time configuration.
     *
     * @param options ROS 2 node options (e.g., parameter overrides).
     */
    explicit BallChaserNode(const rclcpp::NodeOptions &options);

  private:
    // ─── Callbacks ───────────────────────────────────────────────────────────

    /**
     * @brief Routes incoming camera images for processing.
     * @param msg Camera image message.
     */
    void imageCallback(sensor_msgs::msg::Image::SharedPtr msg);

    /**
     * @brief Processes a camera frame: detects ball, updates state machine,
     *        publishes velocity and state as needed.
     * @param msg Camera image message.
     */
    void processFrame(const sensor_msgs::msg::Image::SharedPtr &msg);

    /**
     * @brief Handles a detected ball: updates state machine and publishes velocity.
     * @param region The detected ball region (Left, Center, or Right).
     */
    void handleBallDetected(BallRegion region);

    /**
     * @brief Handles a missing ball: transitions to Lost or evaluates timeout.
     */
    void handleBallLost();

    /**
     * @brief Handles incoming intent commands on the /ball_chaser/command service.
     * @param req Service request containing the command string.
     * @param res Service response indicating acceptance.
     */
    void handleCommand(
        custom_interfaces::srv::BallChaserCommand::Request::SharedPtr req,
        custom_interfaces::srv::BallChaserCommand::Response::SharedPtr res
    );

    // ─── Publishing helpers ──────────────────────────────────────────────────

    /**
     * @brief Publishes a Twist message to /cmd_vel.
     * @param linear Linear velocity (m/s).
     * @param angular Angular velocity (rad/s).
     */
    void publishVelocity(double linear, double angular);

    /**
     * @brief Publishes the current state string to /ball_chaser/state.
     * @param state The behavior state to publish.
     */
    void publishState(BehaviorState state);

    /**
     * @brief Publishes accumulated diagnostics to /diagnostics (called by timer).
     */
    void publishDiagnostics();

    // ─── ROS interfaces ─────────────────────────────────────────────────────

    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr m_cmdVelPub;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr m_statePub;
    rclcpp::Publisher<diagnostic_msgs::msg::DiagnosticArray>::SharedPtr m_diagPub;

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr m_imageSub;

    rclcpp::Service<custom_interfaces::srv::BallChaserCommand>::SharedPtr m_commandService;

    rclcpp::TimerBase::SharedPtr m_diagTimer; ///< 1 Hz diagnostics timer.

    // ─── State machine ──────────────────────────────────────────────────────

    StateMachine m_stateMachine;

    // ─── Diagnostics ────────────────────────────────────────────────────────

    DiagnosticData m_diagnosticData;

    // ─── Detection parameters ───────────────────────────────────────────────

    RgbThreshold m_whiteThreshold; ///< RGB threshold for white ball detection.
    ZoneRatios m_zoneRatios;       ///< Zone boundary ratios for region classification.

    // ─── Velocity parameters ────────────────────────────────────────────────

    double m_linearSpeed;  ///< Forward velocity (m/s).
    double m_angularSpeed; ///< Turn velocity (rad/s).

    // ─── Timeout tracking ───────────────────────────────────────────────────

    uint16_t m_lostTimeoutMs;                              ///< Lost→Idle timeout in milliseconds.
    std::chrono::steady_clock::time_point m_lostEntryTime; ///< Timestamp when Lost state was entered.
};

} // namespace robot_control::ball_chaser
