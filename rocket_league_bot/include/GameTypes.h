#pragma once
#include "Math.h"

// -------------------------------------------------------
// Game constants
// -------------------------------------------------------
namespace RL {

// Field dimensions (Soccar)
constexpr float FIELD_LENGTH  = 10240.f;  // total y length
constexpr float FIELD_WIDTH   = 8192.f;   // total x width
constexpr float FIELD_HEIGHT  = 2044.f;

// Goal
constexpr float GOAL_WIDTH    = 1786.f;
constexpr float GOAL_HEIGHT   = 643.f;
constexpr float GOAL_Y        = 5120.f;   // signed; opponent goal at +GOAL_Y for blue

// Car
constexpr float CAR_MAX_SPEED        = 2300.f;
constexpr float CAR_BOOST_ACC        = 991.666f;
constexpr float CAR_THROTTLE_ACC     = 1600.f;
constexpr float CAR_BRAKE_ACC        = 3500.f;
constexpr float CAR_MAX_BOOST_SPEED  = 2300.f;
constexpr float CAR_SUPERSONIC_SPEED = 2200.f;

// Boost
constexpr float BOOST_MAX      = 100.f;
constexpr float BOOST_CONSUME  = 33.3f;  // per second

// Ball
constexpr float BALL_RADIUS    = 92.75f;
constexpr float BALL_MAX_SPEED = 6000.f;

// Physics tick
constexpr float DT             = 1.f / 120.f;

// -------------------------------------------------------
// Car state
// -------------------------------------------------------
struct CarState {
    Vec3  location;
    Vec3  velocity;
    Vec3  rotation;   // pitch, yaw, roll in radians
    Vec3  angular_velocity;
    float boost;
    bool  is_on_ground;
    bool  is_demolished;
    bool  has_wheel_contact;
    int   team;       // 0 = blue, 1 = orange
};

// -------------------------------------------------------
// Ball state
// -------------------------------------------------------
struct BallState {
    Vec3  location;
    Vec3  velocity;
    Vec3  angular_velocity;
};

// -------------------------------------------------------
// Boost pad
// -------------------------------------------------------
struct BoostPad {
    Vec3  location;
    bool  is_full_boost;  // full = 100 boost, small = 12
    bool  is_active;
    float timer;          // seconds until respawn
};

// -------------------------------------------------------
// Game tick packet - everything our bot sees each frame
// -------------------------------------------------------
struct GamePacket {
    CarState   my_car;
    CarState   opponent;
    BallState  ball;
    BoostPad   boost_pads[34];
    int        num_boost_pads;
    float      time_remaining;
    bool       is_overtime;
    int        my_score;
    int        opponent_score;
};

// -------------------------------------------------------
// Controller output
// -------------------------------------------------------
struct ControllerState {
    float throttle;   // -1 to 1
    float steer;      // -1 to 1
    float pitch;      // -1 to 1
    float yaw;        // -1 to 1
    float roll;       // -1 to 1
    bool  jump;
    bool  boost;
    bool  handbrake;
    bool  use_item;

    ControllerState()
        : throttle(0), steer(0), pitch(0), yaw(0), roll(0),
          jump(false), boost(false), handbrake(false), use_item(false) {}
};

} // namespace RL
