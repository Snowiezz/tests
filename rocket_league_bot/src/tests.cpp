#include "ChampBot.h"
#include <cassert>
#include <cstdio>
#include <cstring>

// Minimal test suite for ChampBot - verifies core behaviours
// without requiring an external test framework.

static int g_pass = 0;
static int g_fail = 0;

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            printf("FAIL: %s  (line %d)\n", #cond, __LINE__); \
            ++g_fail; \
        } else { \
            printf("PASS: %s\n", #cond); \
            ++g_pass; \
        } \
    } while (false)

#define ASSERT_NEAR(a, b, tol) \
    ASSERT_TRUE(std::abs((a) - (b)) <= (tol))

// ---------------------------------------------------------------------------
// Helper: build a minimal packet
// ---------------------------------------------------------------------------
static RL::GamePacket makePacket() {
    RL::GamePacket p{};
    p.my_car.location       = {0.f, 0.f, 17.f};
    p.my_car.rotation       = {0.f, 1.5708f, 0.f};
    p.my_car.boost          = 33.f;
    p.my_car.is_on_ground   = true;
    p.my_car.team           = 0;
    p.opponent.location     = {0.f, 4000.f, 17.f};
    p.opponent.boost        = 33.f;
    p.opponent.is_on_ground = true;
    p.opponent.team         = 1;
    p.ball.location         = {0.f, 0.f, 93.f};
    p.ball.velocity         = {0.f, 0.f, 0.f};
    return p;
}

// ---------------------------------------------------------------------------
// Test 1: Kickoff — bot should boost toward the ball
// ---------------------------------------------------------------------------
static void testKickoff() {
    printf("\n-- Test 1: Kickoff --\n");
    RL::ChampBot bot(0);
    RL::GamePacket pkt = makePacket();
    // Ball at centre, stationary → kickoff condition
    pkt.ball.location = {0.f, 0.f, 93.f};
    pkt.ball.velocity = {0.f, 0.f, 0.f};
    pkt.my_car.location = {-256.f, -3840.f, 17.f};

    auto ctrl = bot.tick(pkt);
    ASSERT_TRUE(ctrl.boost);
    ASSERT_TRUE(ctrl.throttle > 0.f);
}

// ---------------------------------------------------------------------------
// Test 2: Attack — bot should output positive throttle toward opponent goal
// ---------------------------------------------------------------------------
static void testAttack() {
    printf("\n-- Test 2: Attack --\n");
    RL::ChampBot bot(0);
    RL::GamePacket pkt = makePacket();
    pkt.ball.location  = {0.f, 3000.f, 93.f};
    pkt.ball.velocity  = {0.f, 100.f, 0.f};
    pkt.my_car.location = {0.f, 2000.f, 17.f};
    pkt.my_car.boost    = 80.f;
    pkt.opponent.location = {0.f, 4800.f, 17.f};

    auto ctrl = bot.tick(pkt);
    ASSERT_TRUE(ctrl.throttle > 0.f);
}

// ---------------------------------------------------------------------------
// Test 3: Save — ball heading to our goal, bot should use boost to save
// ---------------------------------------------------------------------------
static void testSave() {
    printf("\n-- Test 3: Save --\n");
    RL::ChampBot bot(0);  // Blue defends -y
    RL::GamePacket pkt = makePacket();
    pkt.ball.location  = {0.f, -2000.f, 93.f};
    pkt.ball.velocity  = {0.f, -1500.f, 0.f};  // heading toward blue goal
    pkt.my_car.location = {0.f, 500.f, 17.f};
    pkt.my_car.boost    = 60.f;
    pkt.opponent.location = {200.f, -1000.f, 17.f};

    auto ctrl = bot.tick(pkt);
    ASSERT_TRUE(ctrl.boost);
    ASSERT_TRUE(ctrl.throttle > 0.f);
}

// ---------------------------------------------------------------------------
// Test 4: Rotate back — when opponent is closer to ball
// ---------------------------------------------------------------------------
static void testRotateBack() {
    printf("\n-- Test 4: Rotate Back --\n");
    RL::ChampBot bot(0);
    RL::GamePacket pkt = makePacket();
    // Ball on opponent's half; opponent is much closer
    pkt.ball.location      = {0.f, 3500.f, 93.f};
    pkt.ball.velocity      = {0.f, 0.f, 0.f};
    pkt.my_car.location    = {0.f, -1500.f, 17.f};  // far back
    pkt.my_car.boost       = 80.f;
    pkt.opponent.location  = {0.f, 3200.f, 17.f};  // very close to ball

    auto ctrl = bot.tick(pkt);
    // Bot should be rotating back, still giving some throttle
    ASSERT_TRUE(ctrl.throttle >= 0.f);
}

// ---------------------------------------------------------------------------
// Test 5: Collect boost — low boost and far from ball
// ---------------------------------------------------------------------------
static void testCollectBoost() {
    printf("\n-- Test 5: Collect Boost --\n");
    RL::ChampBot bot(0);
    RL::GamePacket pkt = makePacket();
    pkt.ball.location      = {0.f, 3500.f, 93.f};
    pkt.ball.velocity      = {0.f, 0.f, 0.f};
    pkt.my_car.location    = {100.f, 0.f, 17.f};
    pkt.my_car.boost       = 10.f;  // very low boost
    pkt.opponent.location  = {0.f, 3200.f, 17.f};

    auto ctrl = bot.tick(pkt);
    // Bot should be driving somewhere (throttle > 0)
    ASSERT_TRUE(ctrl.throttle > 0.f);
}

// ---------------------------------------------------------------------------
// Test 6: BallPredictor — ball on floor stays above BALL_RADIUS
// ---------------------------------------------------------------------------
static void testBallPredictor() {
    printf("\n-- Test 6: BallPredictor --\n");
    RL::BallState ball{};
    ball.location = {0.f, 0.f, 93.f};
    ball.velocity = {0.f, 0.f, -500.f};  // heading down fast

    auto slices = RL::BallPredictor::predict(ball, 60);
    ASSERT_TRUE(!slices.empty());
    bool allAboveFloor = true;
    for (const auto& s : slices) {
        if (s.location.z < RL::BALL_RADIUS - 1.f) {
            allAboveFloor = false;
            break;
        }
    }
    ASSERT_TRUE(allAboveFloor);
}

// ---------------------------------------------------------------------------
// Test 7: Orange team — ensure scoring direction is mirrored correctly
// ---------------------------------------------------------------------------
static void testOrangeTeam() {
    printf("\n-- Test 7: Orange Team Kickoff --\n");
    RL::ChampBot bot(1);  // Orange team
    RL::GamePacket pkt = makePacket();
    pkt.my_car.team     = 1;
    pkt.opponent.team   = 0;
    pkt.my_car.location = {256.f, 3840.f, 17.f};
    pkt.ball.location   = {0.f, 0.f, 93.f};
    pkt.ball.velocity   = {0.f, 0.f, 0.f};

    auto ctrl = bot.tick(pkt);
    ASSERT_TRUE(ctrl.boost);
    ASSERT_TRUE(ctrl.throttle > 0.f);
}

// ---------------------------------------------------------------------------
// Test 8: Vec3 maths
// ---------------------------------------------------------------------------
static void testMath() {
    printf("\n-- Test 8: Vec3 Math --\n");
    Vec3 a{1.f, 0.f, 0.f};
    Vec3 b{0.f, 1.f, 0.f};
    ASSERT_NEAR(a.length(), 1.f, 1e-5f);
    ASSERT_NEAR(a.dot(b),   0.f, 1e-5f);
    Vec3 c = a.cross(b);
    ASSERT_NEAR(c.z, 1.f, 1e-5f);
    ASSERT_NEAR(a.dist(b), std::sqrt(2.f), 1e-4f);
}

// ---------------------------------------------------------------------------
int main() {
    testKickoff();
    testAttack();
    testSave();
    testRotateBack();
    testCollectBoost();
    testBallPredictor();
    testOrangeTeam();
    testMath();

    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", g_pass, g_fail);
    printf("========================================\n");
    return g_fail > 0 ? 1 : 0;
}
