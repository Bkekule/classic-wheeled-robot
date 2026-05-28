#pragma once

#include "image_utils.hpp"

#include <cstdint>

namespace robot_control::ball_chaser {

/**
 * @brief Behavioral states for the ball chaser state machine.
 */
enum class BehaviorState : uint8_t {
    Idle,     ///< Robot is stationary, no ball detected.
    Tracking, ///< Robot is actively following the ball.
    Lost      ///< Ball was lost; waiting for reacquisition or timeout.
};

/**
 * @brief Result of a state machine transition attempt.
 */
struct TransitionResult {
    BehaviorState new_state; ///< The state after the transition attempt.
    bool transitioned;       ///< True if the state actually changed.
};

/**
 * @brief Pure C++ state machine for ball chaser behavior control.
 *
 * Manages transitions between Idle, Tracking, and Lost states based on
 * ball detection events, timeouts, and external commands. This class has
 * no ROS dependencies and can be unit tested independently.
 */
class StateMachine {
  public:
    /**
     * @brief Constructs a StateMachine initialized in the Idle state.
     */
    StateMachine();

    /**
     * @brief Returns the current behavior state.
     * @return Current BehaviorState.
     */
    [[nodiscard]] BehaviorState currentState() const;

    /**
     * @brief Handle a ball detection event.
     *
     * Transitions:
     * - Idle → Tracking
     * - Lost → Tracking
     * - Tracking → Tracking (no transition, stays in Tracking)
     *
     * @param region The detected ball region (Left, Center, or Right).
     * @return TransitionResult with the new state and whether a transition occurred.
     */
    TransitionResult onBallDetected(BallRegion region);

    /**
     * @brief Handle a ball-lost event (no ball detected while Tracking).
     *
     * Transitions:
     * - Tracking → Lost
     * - Other states: no change.
     *
     * @return TransitionResult with the new state and whether a transition occurred.
     */
    TransitionResult onBallLost();

    /**
     * @brief Handle a lost-timeout event (timeout expired while in Lost state).
     *
     * Transitions:
     * - Lost → Idle
     * - Other states: no change.
     *
     * @return TransitionResult with the new state and whether a transition occurred.
     */
    TransitionResult onTimeout();

    /**
     * @brief Handle a STOP command from the intent service.
     *
     * Transitions:
     * - Any state → Idle (if not already Idle).
     *
     * @return TransitionResult with the new state and whether a transition occurred.
     */
    TransitionResult onStopCommand();

    /**
     * @brief Handle a movement command (FORWARD, TURN_LEFT, TURN_RIGHT).
     *
     * Transitions:
     * - Any state → Tracking (if not already Tracking).
     *
     * @return TransitionResult with the new state and whether a transition occurred.
     */
    TransitionResult onMovementCommand();

  private:
    BehaviorState m_state; ///< Current behavior state.
};

} // namespace robot_control::ball_chaser
