#include "ChampBot.h"
#include <iostream>
#include <iomanip>
#include <string>

static void printController(const RL::ControllerState& c, int tick) {
    std::cout << "Tick " << std::setw(3) << tick
              << " | throttle=" << std::setw(5) << std::fixed << std::setprecision(2) << c.throttle
              << " steer="      << std::setw(5) << c.steer
              << " pitch="      << std::setw(5) << c.pitch
              << " yaw="        << std::setw(5) << c.yaw
              << " roll="       << std::setw(5) << c.roll
              << " boost="      << (c.boost     ? "Y" : "N")
              << " jump="       << (c.jump      ? "Y" : "N")
              << " handbrake="  << (c.handbrake ? "Y" : "N")
              << "\n";
}

int main() {
    // -------------------------------------------------------
    // Scenario 1: Kickoff
    // -------------------------------------------------------
    std::cout << "=== Scenario 1: Kickoff ===\n";
    {
        RL::ChampBot bot(0); // Blue team

        RL::GamePacket pkt{};
        // Cars at kickoff positions
        pkt.my_car.location       = {-256.f, -3840.f, 17.f};
        pkt.my_car.rotation       = {0.f, 1.5708f, 0.f}; // facing +y
        pkt.my_car.boost          = 33.3f;
        pkt.my_car.is_on_ground   = true;
        pkt.my_car.team           = 0;

        pkt.opponent.location     = {256.f,  3840.f, 17.f};
        pkt.opponent.rotation     = {0.f, -1.5708f, 0.f};
        pkt.opponent.boost        = 33.3f;
        pkt.opponent.is_on_ground = true;
        pkt.opponent.team         = 1;

        // Ball at centre
        pkt.ball.location = {0.f, 0.f, 93.f};
        pkt.ball.velocity = {0.f, 0.f, 0.f};

        for (int i = 0; i < 10; ++i) {
            auto ctrl = bot.tick(pkt);
            printController(ctrl, i);
        }
    }

    // -------------------------------------------------------
    // Scenario 2: Attacking ground shot
    // -------------------------------------------------------
    std::cout << "\n=== Scenario 2: Ground Attack ===\n";
    {
        RL::ChampBot bot(0);

        RL::GamePacket pkt{};
        pkt.my_car.location       = {-500.f, 2000.f, 17.f};
        pkt.my_car.rotation       = {0.f, 1.5708f, 0.f};
        pkt.my_car.velocity       = {0.f, 800.f, 0.f};
        pkt.my_car.boost          = 80.f;
        pkt.my_car.is_on_ground   = true;
        pkt.my_car.team           = 0;

        pkt.opponent.location     = {0.f, 4500.f, 17.f};
        pkt.opponent.boost        = 50.f;
        pkt.opponent.is_on_ground = true;
        pkt.opponent.team         = 1;

        pkt.ball.location = {-300.f, 3000.f, 93.f};
        pkt.ball.velocity = {100.f, 500.f, 0.f};

        for (int i = 0; i < 10; ++i) {
            auto ctrl = bot.tick(pkt);
            printController(ctrl, i);
        }
    }

    // -------------------------------------------------------
    // Scenario 3: Aerial
    // -------------------------------------------------------
    std::cout << "\n=== Scenario 3: Aerial ===\n";
    {
        RL::ChampBot bot(0);

        RL::GamePacket pkt{};
        pkt.my_car.location       = {0.f, 2000.f, 17.f};
        pkt.my_car.rotation       = {0.f, 1.5708f, 0.f};
        pkt.my_car.velocity       = {0.f, 600.f, 0.f};
        pkt.my_car.boost          = 80.f;
        pkt.my_car.is_on_ground   = true;
        pkt.my_car.team           = 0;

        pkt.opponent.location     = {2000.f, 3000.f, 17.f};
        pkt.opponent.boost        = 30.f;
        pkt.opponent.is_on_ground = true;
        pkt.opponent.team         = 1;

        // Ball high in the air heading toward goal
        pkt.ball.location = {0.f, 3500.f, 900.f};
        pkt.ball.velocity = {0.f, 400.f, 200.f};

        for (int i = 0; i < 20; ++i) {
            auto ctrl = bot.tick(pkt);
            printController(ctrl, i);
        }
    }

    // -------------------------------------------------------
    // Scenario 4: Save
    // -------------------------------------------------------
    std::cout << "\n=== Scenario 4: Defensive Save ===\n";
    {
        RL::ChampBot bot(0);

        RL::GamePacket pkt{};
        pkt.my_car.location       = {500.f, 0.f, 17.f};
        pkt.my_car.rotation       = {0.f, -1.5708f, 0.f}; // facing -y
        pkt.my_car.velocity       = {0.f, -200.f, 0.f};
        pkt.my_car.boost          = 50.f;
        pkt.my_car.is_on_ground   = true;
        pkt.my_car.team           = 0;

        pkt.opponent.location     = {-500.f, -2000.f, 17.f};
        pkt.opponent.boost        = 70.f;
        pkt.opponent.is_on_ground = true;
        pkt.opponent.team         = 1;

        // Ball on our half, heading toward our goal at -y
        pkt.ball.location = {200.f, -2500.f, 93.f};
        pkt.ball.velocity = {50.f, -1200.f, 0.f};

        for (int i = 0; i < 10; ++i) {
            auto ctrl = bot.tick(pkt);
            printController(ctrl, i);
        }
    }

    std::cout << "\nChampBot demo complete.\n";
    return 0;
}
