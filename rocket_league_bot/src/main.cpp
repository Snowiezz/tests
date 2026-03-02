#include "ChampBot.h"
#include <iostream>
#include <string>

// Tiny JSON helpers - no external dependencies needed
static float getFloat(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return 0.f;
    pos = json.find(':', pos) + 1;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    return std::stof(json.substr(pos));
}

static bool getBool(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos) + 1;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    return json.substr(pos, 4) == "true";
}

static int getInt(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return 0;
    pos = json.find(':', pos) + 1;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    return std::stoi(json.substr(pos));
}

int main() {
    // Disable buffering so Python gets output immediately
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    setvbuf(stdout, nullptr, _IONBF, 0);

    RL::ChampBot* bot = nullptr;
    std::string line;

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        // Parse JSON packet from Python agent
        RL::GamePacket pkt{};

        // my_car
        auto myPos = line.find("\"my_car\"");
        if (myPos != std::string::npos) {
            auto block = line.substr(myPos);
            pkt.my_car.location.x   = getFloat(block, "x");
            pkt.my_car.location.y   = getFloat(block, "y");
            pkt.my_car.location.z   = getFloat(block, "z");
            pkt.my_car.velocity.x   = getFloat(block, "vx");
            pkt.my_car.velocity.y   = getFloat(block, "vy");
            pkt.my_car.velocity.z   = getFloat(block, "vz");
            pkt.my_car.rotation.x   = getFloat(block, "pitch");
            pkt.my_car.rotation.y   = getFloat(block, "yaw");
            pkt.my_car.rotation.z   = getFloat(block, "roll");
            pkt.my_car.boost        = getFloat(block, "boost");
            pkt.my_car.is_on_ground = getBool(block,  "on_ground");
            pkt.my_car.team         = getInt(block,   "team");
        }

        // opponent
        auto oppPos = line.find("\"opponent\"");
        if (oppPos != std::string::npos) {
            auto block = line.substr(oppPos);
            pkt.opponent.location.x   = getFloat(block, "x");
            pkt.opponent.location.y   = getFloat(block, "y");
            pkt.opponent.location.z   = getFloat(block, "z");
            pkt.opponent.boost        = getFloat(block, "boost");
            pkt.opponent.is_on_ground = getBool(block,  "on_ground");
            pkt.opponent.team         = getInt(block,   "team");
        }

        // ball
        auto ballPos = line.find("\"ball\"");
        if (ballPos != std::string::npos) {
            auto block = line.substr(ballPos);
            pkt.ball.location.x = getFloat(block, "x");
            pkt.ball.location.y = getFloat(block, "y");
            pkt.ball.location.z = getFloat(block, "z");
            pkt.ball.velocity.x = getFloat(block, "vx");
            pkt.ball.velocity.y = getFloat(block, "vy");
            pkt.ball.velocity.z = getFloat(block, "vz");
        }

        // Create bot on first packet (we now know the team)
        if (!bot) {
            bot = new RL::ChampBot(pkt.my_car.team);
        }

        // Run bot logic
        auto ctrl = bot->tick(pkt);

        // Write JSON response back to Python
        std::cout << "{"
            << "\"throttle\":"  << ctrl.throttle  << ","
            << "\"steer\":"     << ctrl.steer      << ","
            << "\"pitch\":"     << ctrl.pitch      << ","
            << "\"yaw\":"       << ctrl.yaw        << ","
            << "\"roll\":"      << ctrl.roll       << ","
            << "\"boost\":"     << (ctrl.boost     ? "true" : "false") << ","
            << "\"jump\":"      << (ctrl.jump      ? "true" : "false") << ","
            << "\"handbrake\":" << (ctrl.handbrake ? "true" : "false")
            << "}\n";
        std::cout.flush();
    }

    delete bot;
    return 0;
}