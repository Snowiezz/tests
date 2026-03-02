#pragma once
#include "GameTypes.h"
#include <vector>

namespace RL {

// -------------------------------------------------------
// Simple physics predictor (ball only, no car)
// Used to predict where the ball will be at a future time.
// -------------------------------------------------------
class BallPredictor {
public:
    struct Slice {
        Vec3  location;
        Vec3  velocity;
        float time;
    };

    // Predict up to `steps` physics steps ahead (DT each).
    // Simplified: treats field walls and floor as flat reflectors.
    static std::vector<Slice> predict(const BallState& ball, int steps = 360) {
        std::vector<Slice> slices;
        slices.reserve(steps);

        Vec3 loc = ball.location;
        Vec3 vel = ball.velocity;

        const float RESTITUTION   = 0.6f;
        const float DRAG          = 0.0305f;   // per-tick drag coefficient
        const float GRAVITY       = 650.f;
        const float HALF_LEN      = FIELD_LENGTH  / 2.f;
        const float HALF_WID      = FIELD_WIDTH   / 2.f;

        for (int i = 0; i < steps; ++i) {
            // Gravity
            vel.z -= GRAVITY * DT;

            // Drag
            vel = vel * (1.f - DRAG * DT);

            // Integrate
            loc = loc + vel * DT;

            // Floor bounce
            if (loc.z < BALL_RADIUS) {
                loc.z = BALL_RADIUS;
                vel.z = std::abs(vel.z) * RESTITUTION;
            }

            // Ceiling
            if (loc.z > FIELD_HEIGHT - BALL_RADIUS) {
                loc.z = FIELD_HEIGHT - BALL_RADIUS;
                vel.z = -std::abs(vel.z) * RESTITUTION;
            }

            // Side walls
            if (loc.x > HALF_WID - BALL_RADIUS) {
                loc.x = HALF_WID - BALL_RADIUS;
                vel.x = -std::abs(vel.x) * RESTITUTION;
            } else if (loc.x < -(HALF_WID - BALL_RADIUS)) {
                loc.x = -(HALF_WID - BALL_RADIUS);
                vel.x = std::abs(vel.x) * RESTITUTION;
            }

            // Back walls (ignore goal opening for simplicity)
            if (loc.y > HALF_LEN - BALL_RADIUS) {
                loc.y = HALF_LEN - BALL_RADIUS;
                vel.y = -std::abs(vel.y) * RESTITUTION;
            } else if (loc.y < -(HALF_LEN - BALL_RADIUS)) {
                loc.y = -(HALF_LEN - BALL_RADIUS);
                vel.y = std::abs(vel.y) * RESTITUTION;
            }

            slices.push_back({loc, vel, static_cast<float>(i + 1) * DT});
        }

        return slices;
    }
};

} // namespace RL
