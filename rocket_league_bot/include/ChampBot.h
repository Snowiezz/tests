#pragma once
#include "GameTypes.h"
#include "BallPredictor.h"
#include <vector>
#include <cmath>

namespace RL {

// -------------------------------------------------------
// Low-level steering helpers
// -------------------------------------------------------

// Returns steer value (-1..1) to align car's yaw toward a target location.
inline float steerToward(const CarState& car, const Vec3& target) {
    // Build forward vector from yaw
    float yaw = car.rotation.y;
    Vec3 forward = {std::cos(yaw), std::sin(yaw), 0.f};
    Vec3 toTarget = (target - car.location).flat().normalized();
    float angle = angle2D(forward, toTarget);
    return clamp(angle * 2.5f, -1.f, 1.f);
}

// Pitch/yaw corrections for aerial alignment toward a 3D target.
// Returns {pitch, yaw, roll} controls.
inline Vec3 aerialAlign(const CarState& car, const Vec3& target) {
    Vec3 diff = (target - car.location).normalized();

    float cp = std::cos(car.rotation.x); // pitch
    float sp = std::sin(car.rotation.x);
    float cy = std::cos(car.rotation.y); // yaw
    float sy = std::sin(car.rotation.y);
    float cr = std::cos(car.rotation.z); // roll
    float sr = std::sin(car.rotation.z);

    // Car axes (simplified rotation matrix columns)
    Vec3 right   = {cy * sp * sr - sy * cr, sy * sp * sr + cy * cr, -cp * sr};
    Vec3 up      = {cy * sp * cr + sy * sr, sy * sp * cr - cy * sr,  cp * cr};

    // Project target direction into local car space to compute angular errors
    float local_x = diff.dot(right);   // positive = target is to the right
    float local_z = diff.dot(up);      // positive = target is above nose

    float pitch_err = local_z;         // pitch up to reach target above
    float yaw_err   = local_x;         // yaw right to face target
    float roll_err  = -up.z;           // keep roof up

    return {
        clamp( pitch_err * 3.0f, -1.f, 1.f),
        clamp( yaw_err   * 3.0f, -1.f, 1.f),
        clamp( roll_err  * 2.0f, -1.f, 1.f)
    };
}

// -------------------------------------------------------
// Drive-to-location helper
// -------------------------------------------------------
inline ControllerState driveToLocation(const CarState& car, const Vec3& target,
                                       bool useBoost = false) {
    ControllerState ctrl;
    float speed  = car.velocity.length();
    float steer  = steerToward(car, target);

    ctrl.throttle = 1.f;
    ctrl.steer    = steer;
    ctrl.boost    = useBoost && (std::abs(steer) < 0.3f) && (speed < CAR_MAX_SPEED - 50.f);

    // Handbrake on tight turns at low speed
    if (std::abs(steer) > 0.9f && speed > 400.f && car.is_on_ground)
        ctrl.handbrake = true;

    return ctrl;
}

// -------------------------------------------------------
// Bot state machine states
// -------------------------------------------------------
enum class BotState {
    KICKOFF,
    COLLECT_BOOST,
    ATTACK,
    AERIAL,
    SHADOW_DEFENSE,
    ROTATE_BACK,
    SAVE
};

// -------------------------------------------------------
// ChampBot - Championship-rank Rocket League AI
//
// Strategy:
//  1. Kickoff - fast kickoff with diagonal approach
//  2. Boost collection when low on boost
//  3. Aerial intercepts when the ball is high and reachable
//  4. Ground shot with power steering toward opponent's goal
//  5. Shadow defense / rotation when teammate is attacking (1v1: rotate back)
//  6. Save when ball is heading toward own goal
// -------------------------------------------------------
class ChampBot {
public:
    explicit ChampBot(int team) : team_(team), state_(BotState::KICKOFF) {}

    // Called every game tick. Returns controller state.
    ControllerState tick(const GamePacket& packet) {
        my_   = packet.my_car;
        opp_  = packet.opponent;
        ball_ = packet.ball;

        // Sign flip: blue defends -y, orange defends +y
        // We normalise so our goal is at -goalDir_ * GOAL_Y
        goalDir_ = (team_ == 0) ? 1.f : -1.f;  // direction to opponent goal

        Vec3 myGoal     = {0.f,  -goalDir_ * GOAL_Y, 0.f};
        Vec3 theirGoal  = {0.f,   goalDir_ * GOAL_Y, 0.f};

        updateState(myGoal, theirGoal, packet);
        return executeState(myGoal, theirGoal);
    }

private:
    int       team_;
    BotState  state_;
    CarState  my_;
    CarState  opp_;
    BallState ball_;
    float     goalDir_;
    bool      aerialJumped_    = false;
    float     aerialJumpTimer_ = 0.f;
    int       jumpFrames_      = 0;

    // -------------------------------------------------------
    // State transition logic
    // -------------------------------------------------------
    void updateState(const Vec3& myGoal, const Vec3& theirGoal,
                     const GamePacket& /*packet*/) {
        // Kickoff detection: ball is at centre and stationary
        if (ball_.location.length2() < 200.f * 200.f &&
            ball_.velocity.length2() < 100.f) {
            state_ = BotState::KICKOFF;
            return;
        }

        // Immediate save check
        if (isBallHeadingToMyGoal(myGoal)) {
            state_ = BotState::SAVE;
            return;
        }

        // Aerial check: ball is high, we can reach it, and it can score
        if (shouldAerial(theirGoal)) {
            state_ = BotState::AERIAL;
            return;
        }

        // Low boost → collect if far from ball action
        float distToBall = my_.location.dist(ball_.location);
        if (my_.boost < 25.f && distToBall > 1500.f) {
            state_ = BotState::COLLECT_BOOST;
            return;
        }

        // Are we the closer car to the ball?
        float myDist  = distToBall;
        float oppDist = opp_.location.dist(ball_.location);

        if (myDist < oppDist + 300.f) {
            state_ = BotState::ATTACK;
        } else {
            state_ = BotState::ROTATE_BACK;
        }
    }

    // -------------------------------------------------------
    // State execution
    // -------------------------------------------------------
    ControllerState executeState(const Vec3& myGoal, const Vec3& theirGoal) {
        switch (state_) {
        case BotState::KICKOFF:       return doKickoff(theirGoal);
        case BotState::SAVE:          return doSave(myGoal);
        case BotState::AERIAL:        return doAerial(theirGoal);
        case BotState::COLLECT_BOOST: return doCollectBoost();
        case BotState::ATTACK:        return doAttack(theirGoal);
        case BotState::ROTATE_BACK:   return doRotateBack(myGoal);
        default:                       return doAttack(theirGoal);
        }
    }

    // -------------------------------------------------------
    // KICKOFF
    // Diagonal approach to beat opponent to the ball
    // -------------------------------------------------------
    ControllerState doKickoff(const Vec3& /*theirGoal*/) {
        // Drive directly at ball, full boost
        ControllerState ctrl = driveToLocation(my_, ball_.location, true);
        ctrl.boost = true;
        ctrl.throttle = 1.f;
        return ctrl;
    }

    // -------------------------------------------------------
    // SAVE
    // Rush back and intercept ball path to own goal
    // -------------------------------------------------------
    ControllerState doSave(const Vec3& myGoal) {
        // Predict where ball crosses goal line
        Vec3 intercept = predictSavePosition(myGoal);
        ControllerState ctrl = driveToLocation(my_, intercept, true);
        ctrl.boost = true;
        return ctrl;
    }

    // -------------------------------------------------------
    // AERIAL
    // Jump + boost toward the ball
    // -------------------------------------------------------
    ControllerState doAerial(const Vec3& theirGoal) {
        ControllerState ctrl;

        // Find intercept point in the air
        Vec3 intercept = findAerialIntercept(theirGoal);

        if (my_.is_on_ground && !aerialJumped_) {
            // First jump
            ctrl.throttle = 1.f;
            ctrl.boost    = true;
            if (jumpFrames_ < 5) {
                ctrl.jump = true;
                jumpFrames_++;
            } else {
                jumpFrames_ = 0;
                aerialJumped_ = true;
                aerialJumpTimer_ = 0.f;
            }
        } else {
            aerialJumpTimer_ += DT;

            Vec3 align = aerialAlign(my_, intercept);
            ctrl.pitch = align.x;
            ctrl.yaw   = align.y;
            ctrl.roll  = align.z;
            ctrl.boost = true;

            // Double jump / flip boost
            if (aerialJumpTimer_ > 0.1f && aerialJumpTimer_ < 0.15f) {
                ctrl.jump = true;
            }

            // Reset when we land or reach the ball
            if (my_.is_on_ground ||
                my_.location.dist(ball_.location) < BALL_RADIUS + 100.f) {
                aerialJumped_    = false;
                aerialJumpTimer_ = 0.f;
                jumpFrames_      = 0;
            }
        }
        return ctrl;
    }

    // -------------------------------------------------------
    // COLLECT_BOOST
    // Drive to nearest active full-boost pad
    // -------------------------------------------------------
    ControllerState doCollectBoost() {
        // This would use packet.boost_pads; for now drive to corner boost positions
        static const Vec3 fullBoostPads[] = {
            {-3072.f, -4096.f, 73.f}, { 3072.f, -4096.f, 73.f},
            {-3584.f,     0.f, 73.f}, { 3584.f,     0.f, 73.f},
            {-3072.f,  4096.f, 73.f}, { 3072.f,  4096.f, 73.f},
        };

        Vec3 best = fullBoostPads[0];
        float bestDist = my_.location.dist(fullBoostPads[0]);
        for (const auto& pad : fullBoostPads) {
            float d = my_.location.dist(pad);
            if (d < bestDist) {
                bestDist = d;
                best = pad;
            }
        }
        return driveToLocation(my_, best, false);
    }

    // -------------------------------------------------------
    // ATTACK
    // Aim ground shot or dribble toward opponent's goal
    // -------------------------------------------------------
    ControllerState doAttack(const Vec3& theirGoal) {
        // Aim for a point slightly behind the ball in the direction of their goal
        Vec3 ballToGoal = (theirGoal - ball_.location).flat().normalized();
        Vec3 hitTarget  = ball_.location - ballToGoal * (BALL_RADIUS + 80.f);

        // If ball is on ground, drive directly through it
        bool ballLow = ball_.location.z < 200.f;
        Vec3 target  = ballLow ? hitTarget : ball_.location;

        float dist  = my_.location.dist(target);
        bool boost  = my_.boost > 20.f && dist > 800.f;

        return driveToLocation(my_, target, boost);
    }

    // -------------------------------------------------------
    // ROTATE_BACK
    // Fall back into defensive position
    // -------------------------------------------------------
    ControllerState doRotateBack(const Vec3& myGoal) {
        // Go to a position between ball and own goal (shadow defense)
        Vec3 ballToGoal = (myGoal - ball_.location).normalized();
        Vec3 shadow = ball_.location + ballToGoal * 1000.f;
        shadow.z = 0.f;

        // Clamp inside field
        shadow.x = clamp(shadow.x, -FIELD_WIDTH / 2.f, FIELD_WIDTH / 2.f);
        shadow.y = clamp(shadow.y, -FIELD_LENGTH / 2.f, FIELD_LENGTH / 2.f);

        return driveToLocation(my_, shadow, false);
    }

    // -------------------------------------------------------
    // Helpers
    // -------------------------------------------------------

    bool isBallHeadingToMyGoal(const Vec3& myGoal) const {
        // Ball velocity pointing toward our goal and ball is on our half
        Vec3 ballToGoal = (myGoal - ball_.location);
        float dot = ball_.velocity.dot(ballToGoal.normalized());
        bool heading = dot > 800.f;
        bool ourHalf = (goalDir_ > 0.f) ? (ball_.location.y < 0.f)
                                         : (ball_.location.y > 0.f);
        return heading && ourHalf;
    }

    bool shouldAerial(const Vec3& theirGoal) const {
        if (my_.boost < 40.f) return false;

        // Ball must be high enough to be worth an aerial
        if (ball_.location.z < 500.f) return false;

        // Must be closer to ball than opponent
        float myDist  = my_.location.dist(ball_.location);
        float oppDist = opp_.location.dist(ball_.location);
        if (myDist > oppDist + 200.f) return false;

        // Ball should be heading toward their goal area (worth attacking)
        float ballFutureY = ball_.location.y + ball_.velocity.y * 0.8f;
        return (goalDir_ > 0.f) ? (ballFutureY > theirGoal.y * 0.4f)
                                 : (ballFutureY < theirGoal.y * 0.4f);
    }

    Vec3 predictSavePosition(const Vec3& /*myGoal*/) const {
        // Simple: predict ball position when it reaches our goal line
        auto slices = BallPredictor::predict(ball_, 240);
        for (const auto& s : slices) {
            bool crossedGoalLine = (goalDir_ > 0.f)
                ? (s.location.y <= -GOAL_Y + 100.f)
                : (s.location.y >=  GOAL_Y - 100.f);
            if (crossedGoalLine) {
                return Vec3(s.location.x, s.location.y, 0.f);
            }
        }
        // Fallback: go to ball
        return ball_.location;
    }

    Vec3 findAerialIntercept(const Vec3& /*theirGoal*/) const {
        auto slices = BallPredictor::predict(ball_, 180);
        for (const auto& s : slices) {
            if (s.location.z < 500.f) continue;

            // Estimate if we can reach this point in time
            float estTime   = my_.location.dist(s.location) / (CAR_MAX_SPEED + CAR_BOOST_ACC * 0.5f);
            if (estTime <= s.time + 0.05f) {
                return s.location;
            }
        }
        return ball_.location;
    }
};

} // namespace RL
