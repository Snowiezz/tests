#pragma once
#include "GameTypes.h"
#include "BallPredictor.h"
#include <vector>
#include <cmath>

namespace RL {

// -------------------------------------------------------
// Low-level steering helpers
// -------------------------------------------------------

inline float steerToward(const CarState& car, const Vec3& target) {
    float yaw = car.rotation.y;
    Vec3 forward = {std::cos(yaw), std::sin(yaw), 0.f};
    Vec3 toTarget = (target - car.location).flat().normalized();
    float angle = angle2D(forward, toTarget);
    return clamp(angle * 2.5f, -1.f, 1.f);
}

inline Vec3 aerialAlign(const CarState& car, const Vec3& target) {
    Vec3 diff = (target - car.location).normalized();

    float cp = std::cos(car.rotation.x);
    float sp = std::sin(car.rotation.x);
    float cy = std::cos(car.rotation.y);
    float sy = std::sin(car.rotation.y);
    float cr = std::cos(car.rotation.z);
    float sr = std::sin(car.rotation.z);

    Vec3 right = {cy * sp * sr - sy * cr, sy * sp * sr + cy * cr, -cp * sr};
    Vec3 up    = {cy * sp * cr + sy * sr, sy * sp * cr - cy * sr,  cp * cr};

    float local_x = diff.dot(right);
    float local_z = diff.dot(up);
    float roll_err = -up.z;

    return {
        clamp( local_z  * 3.0f, -1.f, 1.f),
        clamp( local_x  * 3.0f, -1.f, 1.f),
        clamp( roll_err * 2.0f, -1.f, 1.f)
    };
}

// Returns angle in radians between car forward and target direction (-PI..PI)
inline float angleToTarget(const CarState& car, const Vec3& target) {
    float yaw = car.rotation.y;
    Vec3 forward  = {std::cos(yaw), std::sin(yaw), 0.f};
    Vec3 toTarget = (target - car.location).flat().normalized();
    return angle2D(forward, toTarget);
}

// Drive to a location with smart throttle/boost based on alignment
inline ControllerState driveToLocation(const CarState& car, const Vec3& target,
                                       bool useBoost = false) {
    ControllerState ctrl;
    float speed   = car.velocity.flat().length();
    float steer   = steerToward(car, target);
    float absAngle = std::abs(angleToTarget(car, target));
    float dist    = car.location.flat().dist(target.flat());

    ctrl.steer = steer;

    // If facing away, reverse or turn on spot
    if (absAngle > 2.0f && dist < 500.f) {
        ctrl.throttle = -1.f;
        ctrl.steer    = -steer;
        return ctrl;
    }

    // Slow down when badly misaligned to avoid overshooting
    if (absAngle > 0.5f) {
        ctrl.throttle = clamp(1.f - absAngle, 0.2f, 1.f);
        ctrl.boost    = false;
    } else {
        ctrl.throttle = 1.f;
        ctrl.boost    = useBoost && speed < CAR_MAX_SPEED - 50.f;
    }

    // Handbrake on sharp turns at speed
    if (std::abs(steer) > 0.85f && speed > 500.f && car.is_on_ground)
        ctrl.handbrake = true;

    return ctrl;
}

// -------------------------------------------------------
// Bot states
// -------------------------------------------------------
enum class BotState {
    KICKOFF,
    ATTACK,
    AERIAL,
    COLLECT_BOOST,
    ROTATE_BACK,
    SAVE
};

// -------------------------------------------------------
// ChampBot
// -------------------------------------------------------
class ChampBot {
public:
    explicit ChampBot(int team) : team_(team), state_(BotState::KICKOFF) {}

    ControllerState tick(const GamePacket& packet) {
        my_   = packet.my_car;
        opp_  = packet.opponent;
        ball_ = packet.ball;

        // Blue (team 0) attacks +y, Orange (team 1) attacks -y
        goalDir_   = (team_ == 0) ? 1.f : -1.f;
        Vec3 myGoal    = {0.f, -goalDir_ * GOAL_Y, 0.f};
        Vec3 theirGoal = {0.f,  goalDir_ * GOAL_Y, 0.f};

        updateState(myGoal, theirGoal);
        return executeState(myGoal, theirGoal);
    }

private:
    int       team_;
    BotState  state_;
    CarState  my_;
    CarState  opp_;
    BallState ball_;
    float     goalDir_         = 1.f;
    bool      aerialJumped_    = false;
    float     aerialJumpTimer_ = 0.f;
    int       jumpFrames_      = 0;

    // -------------------------------------------------------
    // State transitions
    // -------------------------------------------------------
    void updateState(const Vec3& myGoal, const Vec3& theirGoal) {
        // Kickoff: ball near centre and nearly stationary
        if (ball_.location.flat().length() < 150.f &&
            ball_.velocity.flat().length() < 100.f) {
            state_ = BotState::KICKOFF;
            return;
        }

        // Save: ball heading toward our goal fast
        if (isBallHeadingToMyGoal(myGoal)) {
            state_ = BotState::SAVE;
            return;
        }

        // Aerial: ball is high and we have boost
        if (shouldAerial(theirGoal)) {
            state_ = BotState::AERIAL;
            return;
        }

        float distToBall = my_.location.dist(ball_.location);
        float oppDist    = opp_.location.dist(ball_.location);

        // Collect boost only if very low AND opponent is far from ball too
        if (my_.boost < 20.f && distToBall > 2000.f && oppDist > 1500.f) {
            state_ = BotState::COLLECT_BOOST;
            return;
        }

        // Attack if we're closer, or rotate back if not
        if (distToBall <= oppDist + 200.f) {
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
        case BotState::KICKOFF:       return doKickoff();
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
    // -------------------------------------------------------
    ControllerState doKickoff() {
        ControllerState ctrl = driveToLocation(my_, ball_.location, true);
        ctrl.boost    = true;
        ctrl.throttle = 1.f;
        return ctrl;
    }

    // -------------------------------------------------------
    // SAVE
    // -------------------------------------------------------
    ControllerState doSave(const Vec3& myGoal) {
        Vec3 intercept = predictSavePosition(myGoal);
        ControllerState ctrl = driveToLocation(my_, intercept, true);
        ctrl.boost = true;
        return ctrl;
    }

    // -------------------------------------------------------
    // AERIAL
    // -------------------------------------------------------
    ControllerState doAerial(const Vec3& theirGoal) {
        ControllerState ctrl;
        Vec3 intercept = findAerialIntercept(theirGoal);

        if (my_.is_on_ground && !aerialJumped_) {
            ctrl.throttle = 1.f;
            ctrl.boost    = true;
            if (jumpFrames_ < 5) {
                ctrl.jump = true;
                jumpFrames_++;
            } else {
                jumpFrames_      = 0;
                aerialJumped_    = true;
                aerialJumpTimer_ = 0.f;
            }
        } else {
            aerialJumpTimer_ += DT;
            Vec3 align = aerialAlign(my_, intercept);
            ctrl.pitch = align.x;
            ctrl.yaw   = align.y;
            ctrl.roll  = align.z;
            ctrl.boost = true;

            if (aerialJumpTimer_ > 0.1f && aerialJumpTimer_ < 0.15f)
                ctrl.jump = true;

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
    // -------------------------------------------------------
    ControllerState doCollectBoost() {
        static const Vec3 fullBoostPads[] = {
            {-3072.f, -4096.f, 73.f}, { 3072.f, -4096.f, 73.f},
            {-3584.f,     0.f, 73.f}, { 3584.f,     0.f, 73.f},
            {-3072.f,  4096.f, 73.f}, { 3072.f,  4096.f, 73.f},
        };

        // Pick closest pad that is on our side of the ball (don't go past ball)
        Vec3 best     = fullBoostPads[0];
        float bestScore = 1e9f;
        for (const auto& pad : fullBoostPads) {
            float d = my_.location.dist(pad);
            // Penalise pads that are past the ball (deep in opponent half)
            float ballDist = ball_.location.dist(pad);
            float score = d + (ballDist < d ? 1000.f : 0.f);
            if (score < bestScore) {
                bestScore = score;
                best = pad;
            }
        }
        return driveToLocation(my_, best, false);
    }

    // -------------------------------------------------------
    // ATTACK - the most important function
    // Approach ball from the correct side to hit toward their goal
    // -------------------------------------------------------
    ControllerState doAttack(const Vec3& theirGoal) {
        float distToBall = my_.location.dist(ball_.location);
        bool  ballLow    = ball_.location.z < 120.f;

        if (!ballLow) {
            // Ball is in the air - just drive to where it will land
            auto slices = BallPredictor::predict(ball_, 60);
            for (const auto& s : slices) {
                if (s.location.z < 120.f) {
                    return driveToLocation(my_, s.location, my_.boost > 30.f);
                }
            }
            // Fallback: drive toward ball
            return driveToLocation(my_, ball_.location, my_.boost > 30.f);
        }

        // --- Ground ball ---
        // Compute the ideal approach direction: from behind the ball
        // relative to their goal, so we hit the ball toward their goal.
        Vec3 goalToBall  = (ball_.location - theirGoal).flat().normalized();
        // Approach point: come from behind the ball (between us and the goal)
        Vec3 approachPt  = ball_.location + goalToBall * (BALL_RADIUS + 120.f);

        // Check if we're already on the correct side to hit
                bool  goodAngle  = std::abs(angleToTarget(my_, approachPt)) < 0.4f;

        Vec3  target;
        bool  useBoost;

        if (distToBall < 400.f && !goodAngle) {
            // Too close and bad angle — back off to approach point
            target   = approachPt;
            useBoost = false;
        } else if (distToBall < 300.f) {
            // Close enough and good angle — aim straight through ball at goal
            Vec3 ballToGoal = (theirGoal - ball_.location).flat().normalized();
            target   = ball_.location + ballToGoal * 50.f;
            useBoost = my_.boost > 15.f;
        } else {
            // Far away — drive to approach point at speed
            target   = approachPt;
            useBoost = my_.boost > 30.f && distToBall > 800.f;
        }

        return driveToLocation(my_, target, useBoost);
    }

    // -------------------------------------------------------
    // ROTATE_BACK
    // -------------------------------------------------------
    ControllerState doRotateBack(const Vec3& myGoal) {
        Vec3 ballToGoal = (myGoal - ball_.location).normalized();
        Vec3 shadow     = ball_.location + ballToGoal * 900.f;
        shadow.z = 0.f;
        shadow.x = clamp(shadow.x, -FIELD_WIDTH  / 2.f, FIELD_WIDTH  / 2.f);
        shadow.y = clamp(shadow.y, -FIELD_LENGTH / 2.f, FIELD_LENGTH / 2.f);
        return driveToLocation(my_, shadow, false);
    }

    // -------------------------------------------------------
    // Helpers
    // -------------------------------------------------------
    bool isBallHeadingToMyGoal(const Vec3& myGoal) const {
        Vec3  toGoal  = (myGoal - ball_.location).normalized();
        float dot     = ball_.velocity.dot(toGoal);
        bool  heading = dot > 200.f;
        bool  ourHalf = (goalDir_ > 0.f) ? (ball_.location.y < 500.f)
                                          : (ball_.location.y > -500.f);
        return heading && ourHalf;
    }

    bool shouldAerial(const Vec3& theirGoal) const {
        if (my_.boost < 40.f)           return false;
        if (ball_.location.z < 500.f)   return false;
        float myDist  = my_.location.dist(ball_.location);
        float oppDist = opp_.location.dist(ball_.location);
        if (myDist > oppDist + 200.f)   return false;
        float ballFutureY = ball_.location.y + ball_.velocity.y * 0.8f;
        return (goalDir_ > 0.f) ? (ballFutureY > theirGoal.y * 0.4f)
                                 : (ballFutureY < theirGoal.y * 0.4f);
    }

    Vec3 predictSavePosition(const Vec3& /*myGoal*/) const {
        auto slices = BallPredictor::predict(ball_, 240);
        for (const auto& s : slices) {
            bool crossed = (goalDir_ > 0.f)
                ? (s.location.y <= -GOAL_Y + 100.f)
                : (s.location.y >=  GOAL_Y - 100.f);
            if (crossed) return {s.location.x, s.location.y, 0.f};
        }
        return ball_.location;
    }

    Vec3 findAerialIntercept(const Vec3& /*theirGoal*/) const {
        auto slices = BallPredictor::predict(ball_, 180);
        for (const auto& s : slices) {
            if (s.location.z < 500.f) continue;
            float estTime = my_.location.dist(s.location) /
                            (CAR_MAX_SPEED + CAR_BOOST_ACC * 0.5f);
            if (estTime <= s.time + 0.05f) return s.location;
        }
        return ball_.location;
    }
};

} // namespace RL



