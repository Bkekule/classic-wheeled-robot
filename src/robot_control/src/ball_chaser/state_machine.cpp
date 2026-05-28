#include "state_machine.hpp"

#include <cassert>

namespace robot_control::ball_chaser {

StateMachine::StateMachine() : m_state(BehaviorState::Idle) {}

BehaviorState StateMachine::currentState() const { return m_state; }

TransitionResult StateMachine::onBallDetected(BallRegion region) {
    assert(region != BallRegion::NotFound && "onBallDetected called with NotFound region");

    switch (m_state) {
    case BehaviorState::Idle:
    case BehaviorState::Lost:
        m_state = BehaviorState::Tracking;
        return {m_state, true};
    case BehaviorState::Tracking:
        return {m_state, false};
    }
}

TransitionResult StateMachine::onBallLost() {
    if (m_state == BehaviorState::Tracking) {
        m_state = BehaviorState::Lost;
        return {m_state, true};
    }
    return {m_state, false};
}

TransitionResult StateMachine::onTimeout() {
    if (m_state == BehaviorState::Lost) {
        m_state = BehaviorState::Idle;
        return {m_state, true};
    }
    return {m_state, false};
}

TransitionResult StateMachine::onStopCommand() {
    if (m_state != BehaviorState::Idle) {
        m_state = BehaviorState::Idle;
        return {m_state, true};
    }
    return {m_state, false};
}

TransitionResult StateMachine::onMovementCommand() {
    if (m_state != BehaviorState::Tracking) {
        m_state = BehaviorState::Tracking;
        return {m_state, true};
    }
    return {m_state, false};
}

} // namespace robot_control::ball_chaser
