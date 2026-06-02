/**
 * @file test_state_machine.cpp
 * @brief Property-based tests for the ball chaser StateMachine class.
 *
 * Uses rapidcheck + GTest to verify state machine correctness properties
 * as defined in the ball-chaser-refactor design document.
 *
 * Feature: ball-chaser-refactor
 */

#include "state_machine.hpp"

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

using namespace robot_control::ball_chaser;

// ─────────────────────────────────────────────────────────────────────────────
// Generators
// ─────────────────────────────────────────────────────────────────────────────

namespace rc {

/** @brief Arbitrary generator for BallRegion (excluding NotFound). */
template <> struct Arbitrary<BallRegion> {
    /** @brief Generates a random valid BallRegion for detection events. */
    static Gen<BallRegion> arbitrary() { return gen::element(BallRegion::Left, BallRegion::Center, BallRegion::Right); }
};

} // namespace rc

// ─────────────────────────────────────────────────────────────────────────────
// Property 1: Ball detection triggers Tracking transition
// Feature: ball-chaser-refactor, Property 1: Ball detection triggers Tracking
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Test fixture for state machine property tests. */
class StateMachinePropertyTest : public ::testing::Test {};

/** @brief Property 1: Ball detection from Idle or Lost always transitions to Tracking. */
RC_GTEST_PROP(StateMachinePropertyTest, BallDetectionFromIdleOrLostTransitionsToTracking, (BallRegion region)) {
    // Feature: ball-chaser-refactor, Property 1: Ball detection triggers Tracking
    auto startInIdle_ = *rc::gen::arbitrary<bool>();

    StateMachine sm_;

    if (!startInIdle_) {
        // Force into Lost state: Idle → Tracking → Lost
        sm_.onBallDetected(BallRegion::Center);
        sm_.onBallLost();
        RC_ASSERT(sm_.currentState() == BehaviorState::Lost);
    } else {
        RC_ASSERT(sm_.currentState() == BehaviorState::Idle);
    }

    auto result_ = sm_.onBallDetected(region);

    RC_ASSERT(result_.new_state == BehaviorState::Tracking);
    RC_ASSERT(result_.transitioned == true);
}

// ─────────────────────────────────────────────────────────────────────────────
// Property 2: Velocity mapping correctness (state machine perspective)
// Feature: ball-chaser-refactor, Property 2: Tracking state stays in Tracking
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Property 2: Already-Tracking state stays in Tracking on subsequent detection. */
RC_GTEST_PROP(StateMachinePropertyTest, AlreadyTrackingStaysInTrackingOnBallDetected, (BallRegion region)) {
    // Feature: ball-chaser-refactor, Property 2: Velocity mapping correctness
    // Verifies: for any region while in Tracking, onBallDetected returns
    // transitioned=false, confirming the caller knows to just publish velocity.
    StateMachine sm_;
    sm_.onBallDetected(BallRegion::Center); // Enter Tracking
    RC_ASSERT(sm_.currentState() == BehaviorState::Tracking);

    auto result_ = sm_.onBallDetected(region);

    RC_ASSERT(result_.new_state == BehaviorState::Tracking);
    RC_ASSERT(result_.transitioned == false);
}

// ─────────────────────────────────────────────────────────────────────────────
// Property 3: Ball disappearance triggers Lost transition
// Feature: ball-chaser-refactor, Property 3: Ball disappearance triggers Lost
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Property 3: Ball disappearance from Tracking transitions to Lost. */
RC_GTEST_PROP(StateMachinePropertyTest, BallDisappearanceFromTrackingTransitionsToLost, (BallRegion region)) {
    // Feature: ball-chaser-refactor, Property 3: Ball disappearance triggers Lost
    StateMachine sm_;
    sm_.onBallDetected(region); // Enter Tracking from Idle
    RC_ASSERT(sm_.currentState() == BehaviorState::Tracking);

    auto result_ = sm_.onBallLost();

    RC_ASSERT(result_.new_state == BehaviorState::Lost);
    RC_ASSERT(result_.transitioned == true);
}

// ─────────────────────────────────────────────────────────────────────────────
// Property 4: Lost timeout triggers Idle
// Feature: ball-chaser-refactor, Property 4: Lost timeout triggers Idle
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Property 4: Lost timeout transitions state machine to Idle. */
RC_GTEST_PROP(StateMachinePropertyTest, LostTimeoutTransitionsToIdle, (BallRegion region)) {
    // Feature: ball-chaser-refactor, Property 4: Lost timeout triggers Idle
    StateMachine sm_;
    sm_.onBallDetected(region); // Idle → Tracking
    sm_.onBallLost();           // Tracking → Lost
    RC_ASSERT(sm_.currentState() == BehaviorState::Lost);

    auto result_ = sm_.onTimeout();

    RC_ASSERT(result_.new_state == BehaviorState::Idle);
    RC_ASSERT(result_.transitioned == true);
}

// ─────────────────────────────────────────────────────────────────────────────
// Property 5: State transitions are exhaustive (no undefined state reachable)
// Feature: ball-chaser-refactor, Property 5: State transitions are exhaustive
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Property 5: Random event sequences never reach an undefined state. */
RC_GTEST_PROP(StateMachinePropertyTest, RandomEventSequenceNeverReachesUndefinedState, ()) {
    // Feature: ball-chaser-refactor, Property 5: State transitions are exhaustive
    auto eventCount_ = *rc::gen::inRange(1, 100);

    StateMachine sm_;

    for (int i_ = 0; i_ < eventCount_; ++i_) {
        auto eventType_ = *rc::gen::inRange(0, 5);

        switch (eventType_) {
        case 0: {
            auto region_ = *rc::gen::arbitrary<BallRegion>();
            sm_.onBallDetected(region_);
            break;
        }
        case 1:
            sm_.onBallLost();
            break;
        case 2:
            sm_.onTimeout();
            break;
        case 3:
            sm_.onStopCommand();
            break;
        case 4:
            sm_.onMovementCommand();
            break;
        default:
            break;
        }

        auto state_ = sm_.currentState();
        RC_ASSERT(state_ == BehaviorState::Idle || state_ == BehaviorState::Tracking || state_ == BehaviorState::Lost);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Property 8: onBallDetected with valid region from Idle (positive case)
// Feature: ball-chaser-refactor, Property 8: onBallDetected with NotFound is assert
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Example-based test verifying valid region from Idle transitions to Tracking. */
TEST(StateMachineExampleTest, BallDetectedWithValidRegionFromIdleTransitionsToTracking) {
    // Feature: ball-chaser-refactor, Property 8: onBallDetected with NotFound is assert
    // Note: NotFound case triggers an assert and cannot be tested in release mode.
    // This positive-case test verifies the valid path works correctly.
    StateMachine sm_;
    RC_ASSERT(sm_.currentState() == BehaviorState::Idle);

    auto resultLeft_ = sm_.onBallDetected(BallRegion::Left);
    EXPECT_EQ(resultLeft_.new_state, BehaviorState::Tracking);
    EXPECT_TRUE(resultLeft_.transitioned);

    // Reset via stop command
    sm_.onStopCommand();
    EXPECT_EQ(sm_.currentState(), BehaviorState::Idle);

    auto resultCenter_ = sm_.onBallDetected(BallRegion::Center);
    EXPECT_EQ(resultCenter_.new_state, BehaviorState::Tracking);
    EXPECT_TRUE(resultCenter_.transitioned);

    // Reset via stop command
    sm_.onStopCommand();
    EXPECT_EQ(sm_.currentState(), BehaviorState::Idle);

    auto resultRight_ = sm_.onBallDetected(BallRegion::Right);
    EXPECT_EQ(resultRight_.new_state, BehaviorState::Tracking);
    EXPECT_TRUE(resultRight_.transitioned);
}

// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Test entry point. Runs all GTest + rapidcheck property tests for the state machine. */
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
