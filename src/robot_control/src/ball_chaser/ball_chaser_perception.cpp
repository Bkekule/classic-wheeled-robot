/**
 * @file ball_chaser_perception.cpp
 * @brief Image processing pipeline: camera callback, ball detection, and
 *        state machine driving based on detection results.
 */

#include "ball_chaser_node.hpp"

#include <chrono>
#include <string>

namespace robot_control::ball_chaser {

namespace {

/**
 * @brief Derives bytes-per-pixel from a ROS image encoding string.
 */
uint8_t encodingToBpp(const std::string &encoding) {
    if (encoding == "rgb8" || encoding == "bgr8") {
        return 3;
    }
    if (encoding == "rgba8" || encoding == "bgra8") {
        return 4;
    }
    return 0;
}

} // anonymous namespace

// NOLINTBEGIN(performance-unnecessary-value-param)
void BallChaserNode::imageCallback(sensor_msgs::msg::Image::SharedPtr msg) { processFrame(msg); }
// NOLINTEND(performance-unnecessary-value-param)

// NOLINTBEGIN(readability-function-cognitive-complexity)
void BallChaserNode::processFrame(const sensor_msgs::msg::Image::SharedPtr &msg) {
    const uint8_t bpp_ = encodingToBpp(msg->encoding);
    if (bpp_ == 0) {
        RCLCPP_WARN_ONCE(this->get_logger(), "Unsupported image encoding '%s'. Skipping frame.", msg->encoding.c_str());
        return;
    }

    if (msg->data.empty()) {
        RCLCPP_WARN(this->get_logger(), "Image data is empty. Skipping frame.");
        return;
    }

    if (msg->step < msg->width * bpp_) {
        RCLCPP_WARN(
            this->get_logger(), "Image step (%u) < width (%u) * bpp (%u). Skipping frame.", msg->step, msg->width, bpp_
        );
        return;
    }

    const ImageDimensions dims_{msg->width, msg->height, msg->step, bpp_};
    const BallRegion region_ = findBallRegion(msg->data.data(), dims_, m_whiteThreshold, m_zoneRatios);

    m_diagnosticData.frames_processed++;
    m_diagnosticData.last_detection_region = region_;

    if (region_ != BallRegion::NotFound) {
        handleBallDetected(region_);
    } else {
        handleBallLost();
    }
}

void BallChaserNode::handleBallDetected(BallRegion region) {
    m_diagnosticData.detections_since_last_report++;

    const auto result_ = m_stateMachine.onBallDetected(region);
    if (result_.transitioned) {
        publishState(result_.new_state);
    }

    switch (region) {
    case BallRegion::Left:
        publishVelocity(0.0, m_angularSpeed);
        break;
    case BallRegion::Center:
        publishVelocity(m_linearSpeed, 0.0);
        break;
    case BallRegion::Right:
        publishVelocity(0.0, -m_angularSpeed);
        break;
    case BallRegion::NotFound:
        break; // unreachable
    }

    RCLCPP_INFO_THROTTLE(
        this->get_logger(), *this->get_clock(), 1000, "Ball detected in region: %s",
        region == BallRegion::Left     ? "Left"
        : region == BallRegion::Center ? "Center"
                                       : "Right"
    );
}
// NOLINTEND(readability-function-cognitive-complexity)

void BallChaserNode::handleBallLost() {
    const auto currentState_ = m_stateMachine.currentState();

    if (currentState_ == BehaviorState::Tracking) {
        m_stateMachine.onBallLost();
        m_lostEntryTime = std::chrono::steady_clock::now();
        publishState(BehaviorState::Lost);
    } else if (currentState_ == BehaviorState::Lost) {
        const auto elapsed_ = std::chrono::steady_clock::now() - m_lostEntryTime;
        const auto elapsedMs_ = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_).count();

        if (elapsedMs_ > m_lostTimeoutMs) {
            m_stateMachine.onTimeout();
            publishVelocity(0.0, 0.0);
            publishState(BehaviorState::Idle);
        }
    }
    // Idle: do nothing
}

} // namespace robot_control::ball_chaser
