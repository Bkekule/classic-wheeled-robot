/**
 * @file ball_chaser_actuation.cpp
 * @brief Output side: velocity publishing, state publishing, diagnostics
 *        reporting, and intent service handling.
 */

#include "ball_chaser_node.hpp"

#include <string>

namespace robot_control::ball_chaser {

namespace {

/**
 * @brief Converts a BehaviorState enum to its string representation.
 */
const char *stateToString(BehaviorState state) {
    switch (state) {
    case BehaviorState::Idle:
        return "Idle";
    case BehaviorState::Tracking:
        return "Tracking";
    case BehaviorState::Lost:
        return "Lost";
    }
    return "Unknown";
}

/**
 * @brief Converts a BallRegion enum to its string representation.
 */
const char *regionToString(BallRegion region) {
    switch (region) {
    case BallRegion::Left:
        return "Left";
    case BallRegion::Center:
        return "Center";
    case BallRegion::Right:
        return "Right";
    case BallRegion::NotFound:
        return "NotFound";
    }
    return "Unknown";
}

} // anonymous namespace

// ─── Velocity publishing ─────────────────────────────────────────────────────

// NOLINTBEGIN(readability-function-cognitive-complexity)
void BallChaserNode::publishVelocity(double linear, double angular) {
    auto twist = geometry_msgs::msg::Twist();
    twist.linear.x = linear;
    twist.angular.z = angular;
    m_cmdVelPub->publish(twist);

    RCLCPP_INFO_THROTTLE(
        this->get_logger(), *this->get_clock(), 1000, "Publishing velocity: linear=%.2f, angular=%.2f", linear, angular
    );
}
// NOLINTEND(readability-function-cognitive-complexity)

// ─── State publishing ────────────────────────────────────────────────────────

void BallChaserNode::publishState(BehaviorState state) {
    auto msg = std_msgs::msg::String();
    msg.data = stateToString(state);
    m_statePub->publish(msg);
}

// ─── Diagnostics ─────────────────────────────────────────────────────────────

void BallChaserNode::publishDiagnostics() {
    auto diag_array = diagnostic_msgs::msg::DiagnosticArray();
    diag_array.header.stamp = this->now();

    auto status = diagnostic_msgs::msg::DiagnosticStatus();
    status.name = "ball_chaser/ball_chaser_node";
    status.level = diagnostic_msgs::msg::DiagnosticStatus::OK;
    status.message = stateToString(m_stateMachine.currentState());

    auto addKv = [&status](const char *key, const std::string &value) {
        diagnostic_msgs::msg::KeyValue entry;
        entry.key = key;
        entry.value = value;
        status.values.push_back(entry);
    };

    addKv("frames_processed", std::to_string(m_diagnosticData.frames_processed));
    addKv("current_state", stateToString(m_stateMachine.currentState()));
    addKv("detections_since_last_report", std::to_string(m_diagnosticData.detections_since_last_report));
    addKv("last_detection_region", regionToString(m_diagnosticData.last_detection_region));

    diag_array.status.push_back(status);
    m_diagPub->publish(diag_array);

    m_diagnosticData.detections_since_last_report = 0;
}

// ─── Intent service ──────────────────────────────────────────────────────────
// NOLINTBEGIN(readability-function-cognitive-complexity, performance-unnecessary-value-param)
void BallChaserNode::handleCommand(
    const custom_interfaces::srv::BallChaserCommand::Request::SharedPtr req,
    const custom_interfaces::srv::BallChaserCommand::Response::SharedPtr res
) {
    auto executeMovement = [this](double linear, double angular) {
        publishVelocity(linear, angular);
        auto result = m_stateMachine.onMovementCommand();
        if (result.transitioned) {
            publishState(result.new_state);
        }
    };

    if (req->command == "STOP") {
        publishVelocity(0.0, 0.0);
        auto result = m_stateMachine.onStopCommand();
        if (result.transitioned) {
            publishState(result.new_state);
        }
        res->accepted = true;
    } else if (req->command == "FORWARD") {
        executeMovement(m_linearSpeed, 0.0);
        res->accepted = true;
    } else if (req->command == "TURN_LEFT") {
        executeMovement(0.0, m_angularSpeed);
        res->accepted = true;
    } else if (req->command == "TURN_RIGHT") {
        executeMovement(0.0, -m_angularSpeed);
        res->accepted = true;
    } else {
        res->accepted = false;
        RCLCPP_WARN(this->get_logger(), "Unrecognized command: '%s'", req->command.c_str());
    }
}
// NOLINTEND(readability-function-cognitive-complexity, performance-unnecessary-value-param)
} // namespace robot_control::ball_chaser
