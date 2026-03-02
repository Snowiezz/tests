# Rocket League ChampBot (C++)

A championship-rank Rocket League bot written in C++17, designed to operate at **Champion** level or above.

## Features

| Capability | Description |
|---|---|
| **Kickoff** | Diagonal fast-start directly toward the ball with full boost |
| **Ground shots** | Aims through the ball toward the opponent's goal |
| **Aerials** | Jumps + boosts to intercept high balls, with dual-axis aerial alignment |
| **Defensive saves** | Physics-predicted ball interception at the goal line |
| **Boost management** | Collects full-boost pads (100 boost) when critically low |
| **Rotation** | Shadow defense / retreat when the opponent is closer to the ball |
| **Ball prediction** | 3-second physics rollout (gravity, drag, wall/floor bounces) |

## Project structure

```
rocket_league_bot/
├── CMakeLists.txt          # CMake build (C++17)
├── include/
│   ├── Math.h              # Vec3 and scalar helpers
│   ├── GameTypes.h         # Game constants, car/ball/packet structs, ControllerState
│   ├── BallPredictor.h     # Lightweight ball physics rollout
│   └── ChampBot.h          # Main bot AI (state machine + all behaviour logic)
└── src/
    ├── main.cpp            # Demo driver – simulates 4 game scenarios
    └── tests.cpp           # Self-contained unit tests (no external framework)
```

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run the demo

```bash
./build/champ_bot_demo
```

The demo runs four scenarios (kickoff, ground attack, aerial, defensive save) and
prints the controller output for each tick.

## Run the tests

```bash
./build/champ_bot_tests
# or via CTest:
ctest --test-dir build --output-on-failure
```

All 8 test functions (15 individual assertions) should pass with 0 failures.

## Integration with RLBot

In a real RLBot integration (`rlbot.cfg` + flatbuffers/gRPC), replace the `main.cpp`
driver with a proper `RLBotInterface` loop that feeds a `GamePacket` from the framework
on each tick and sends back the returned `ControllerState`.

The `ChampBot::tick(const GamePacket&)` method is the sole entry point and is
thread-safe (no global state).

## AI design

`ChampBot` uses a **priority-ordered state machine** evaluated every tick:

1. **Kickoff** – triggered when the ball is stationary at centre-field.
2. **Save** – pre-empts all other states when the ball's velocity vector points toward our goal.
3. **Aerial** – activated when the ball is above 500 uu, we have ≥40 boost, and we are the closer car.
4. **Collect Boost** – triggered when boost < 25 and we are > 1500 uu from the ball.
5. **Attack** – default offensive state; steers through the ball toward the opponent's goal.
6. **Rotate Back** – shadow defense when the opponent is closer to the ball.
