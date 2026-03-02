import numpy as np
import torch
import torch.nn as nn
from stable_baselines3 import PPO
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.callbacks import CheckpointCallback
import gymnasium as gym
from gymnasium import spaces
import RocketSim as rsim
import math
import os

# -------------------------------------------------------
# Constants (match your GameTypes.h)
# -------------------------------------------------------
FIELD_LENGTH  = 10240.0
FIELD_WIDTH   = 8192.0
FIELD_HEIGHT  = 2044.0
GOAL_Y        = 5120.0
CAR_MAX_SPEED = 2300.0
BALL_MAX_SPEED = 6000.0
BOOST_MAX     = 100.0
BALL_RADIUS   = 92.75

# -------------------------------------------------------
# Observation space: 23 floats (normalised)
# car pos(3), car vel(3), car rot(3), car ang_vel(3),
# car boost(1), on_ground(1),
# ball pos(3), ball vel(3), ball ang_vel(3) = 23 total
# -------------------------------------------------------
OBS_SIZE = 23

# -------------------------------------------------------
# Action space: 8 continuous values
# throttle, steer, pitch, yaw, roll [-1,1]
# jump, boost, handbrake [0,1] (threshold at 0.5)
# -------------------------------------------------------
ACT_SIZE = 8


def get_obs(car: rsim.CarState, ball: rsim.BallState) -> np.ndarray:
    pos  = car.pos
    vel  = car.vel
    rot  = car.rot_mat  # 3x3 rotation matrix
    avel = car.ang_vel
    bpos = ball.pos
    bvel = ball.vel
    bavel= ball.ang_vel

    # Forward vector from rotation matrix (row 0)
    fwd = [rot[0][0], rot[0][1], rot[0][2]]

    obs = np.array([
        pos[0] / (FIELD_WIDTH  / 2),
        pos[1] / (FIELD_LENGTH / 2),
        pos[2] / FIELD_HEIGHT,
        vel[0] / CAR_MAX_SPEED,
        vel[1] / CAR_MAX_SPEED,
        vel[2] / CAR_MAX_SPEED,
        fwd[0], fwd[1], fwd[2],           # forward direction
        avel[0] / 5.5, avel[1] / 5.5, avel[2] / 5.5,
        car.boost / BOOST_MAX,
        float(car.is_on_ground),
        bpos[0] / (FIELD_WIDTH  / 2),
        bpos[1] / (FIELD_LENGTH / 2),
        bpos[2] / FIELD_HEIGHT,
        bvel[0] / BALL_MAX_SPEED,
        bvel[1] / BALL_MAX_SPEED,
        bvel[2] / BALL_MAX_SPEED,
        bavel[0] / 6.0, bavel[1] / 6.0, bavel[2] / 6.0,
    ], dtype=np.float32)
    return obs


def compute_reward(car: rsim.CarState, ball: rsim.BallState,
                   prev_ball: rsim.BallState, team: int) -> float:
    reward = 0.0
    goal_dir = 1.0 if team == 0 else -1.0

    # --- Ball toward opponent goal ---
    ball_vel_y = ball.vel[1] * goal_dir
    reward += ball_vel_y / BALL_MAX_SPEED * 2.0

    # --- Car facing ball ---
    car_pos  = np.array(car.pos[:2])
    ball_pos = np.array(ball.pos[:2])
    to_ball  = ball_pos - car_pos
    dist     = np.linalg.norm(to_ball) + 1e-6
    to_ball_n = to_ball / dist

    rot  = car.rot_mat
    fwd  = np.array([rot[0][0], rot[0][1]])
    fwd_n = fwd / (np.linalg.norm(fwd) + 1e-6)
    face_reward = float(np.dot(fwd_n, to_ball_n))
    reward += face_reward * 0.3

    # --- Closing distance to ball ---
    prev_dist = np.linalg.norm(
        np.array(car.pos[:2]) - np.array(prev_ball.pos[:2]))
    curr_dist = dist
    reward += (prev_dist - curr_dist) / CAR_MAX_SPEED * 0.5

    # --- Goal scored ---
    opp_goal_y = GOAL_Y * goal_dir
    if abs(ball.pos[1]) > GOAL_Y - 100:
        if ball.pos[1] * goal_dir > 0:
            reward += 10.0   # we scored
        else:
            reward -= 10.0   # they scored

    # --- Boost efficiency ---
    reward += car.boost / BOOST_MAX * 0.05

    return reward


# -------------------------------------------------------
# Gym Environment
# -------------------------------------------------------
class RLBotEnv(gym.Env):
    metadata = {"render_modes": []}

    def __init__(self, team: int = 0):
        super().__init__()
        self.team = team
        self.observation_space = spaces.Box(
            low=-1.0, high=1.0, shape=(OBS_SIZE,), dtype=np.float32)
        self.action_space = spaces.Box(
            low=-1.0, high=1.0, shape=(ACT_SIZE,), dtype=np.float32)

        # Init RocketSim arena
        rsim.init()
        self.arena = rsim.Arena(rsim.GameMode.SOCCAR)

        # Add cars
        self.car  = self.arena.add_car(rsim.Team.BLUE  if team == 0 else rsim.Team.ORANGE)
        self.opp  = self.arena.add_car(rsim.Team.ORANGE if team == 0 else rsim.Team.BLUE)

        self.prev_ball = None
        self.steps     = 0
        self.max_steps = 3000  # ~25 seconds at 120hz

    def _randomise(self):
        """Randomise car and ball positions for variety."""
        # Ball: random position in field
        ball_state = rsim.BallState()
        ball_state.pos = rsim.Vec(
            np.random.uniform(-2000, 2000),
            np.random.uniform(-3000, 3000),
            BALL_RADIUS + 20
        )
        ball_state.vel = rsim.Vec(
            np.random.uniform(-500, 500),
            np.random.uniform(-500, 500),
            np.random.uniform(0, 200)
        )
        self.arena.ball.set_state(ball_state)

        # Our car
        car_state = rsim.CarState()
        car_state.pos = rsim.Vec(
            np.random.uniform(-2000, 2000),
            np.random.uniform(-4000, 0) * (1 if self.team == 0 else -1),
            17
        )
        car_state.boost = np.random.uniform(30, 100)
        self.car.set_state(car_state)

        # Opponent: random on their side
        opp_state = rsim.CarState()
        opp_state.pos = rsim.Vec(
            np.random.uniform(-2000, 2000),
            np.random.uniform(0, 4000) * (1 if self.team == 0 else -1),
            17
        )
        opp_state.boost = np.random.uniform(30, 100)
        self.opp.set_state(opp_state)

    def reset(self, seed=None, options=None):
        super().reset(seed=seed)
        self._randomise()
        self.steps = 0
        ball  = self.arena.ball.get_state()
        car   = self.car.get_state()
        self.prev_ball = ball
        obs = get_obs(car, ball)
        return obs, {}

    def step(self, action):
        # Parse action
        controls = rsim.CarControls()
        controls.throttle  = float(np.clip(action[0], -1, 1))
        controls.steer     = float(np.clip(action[1], -1, 1))
        controls.pitch     = float(np.clip(action[2], -1, 1))
        controls.yaw       = float(np.clip(action[3], -1, 1))
        controls.roll      = float(np.clip(action[4], -1, 1))
        controls.jump      = bool(action[5] > 0.0)
        controls.boost     = bool(action[6] > 0.0)
        controls.handbrake = bool(action[7] > 0.0)

        self.car.set_controls(controls)

        # Simple opponent: drive toward ball
        opp_state  = self.opp.get_state()
        ball_state = self.arena.ball.get_state()
        opp_ctrl   = rsim.CarControls()
        opp_ctrl.throttle = 1.0
        opp_ctrl.boost    = True
        self.opp.set_controls(opp_ctrl)

        # Step simulation (4 ticks = ~33ms)
        self.arena.step(4)

        car  = self.car.get_state()
        ball = self.arena.ball.get_state()

        reward   = compute_reward(car, ball, self.prev_ball, self.team)
        self.prev_ball = ball
        self.steps    += 1

        obs       = get_obs(car, ball)
        truncated = self.steps >= self.max_steps
        terminated = abs(ball.pos[1]) > GOAL_Y - 50  # goal scored

        return obs, reward, terminated, truncated, {}

    def close(self):
        pass


# -------------------------------------------------------
# Train
# -------------------------------------------------------
if __name__ == "__main__":
    os.makedirs("models", exist_ok=True)
    os.makedirs("logs",   exist_ok=True)

    print("Creating environment...")
    env = make_vec_env(lambda: RLBotEnv(team=0), n_envs=4)

    print("Creating PPO model...")
    model = PPO(
        "MlpPolicy",
        env,
        verbose=1,
        learning_rate=3e-4,
        n_steps=2048,
        batch_size=256,
        n_epochs=10,
        gamma=0.99,
        gae_lambda=0.95,
        clip_range=0.2,
        tensorboard_log="./logs/",
        policy_kwargs=dict(
            net_arch=[256, 256, 128],
            activation_fn=nn.ReLU
        )
    )

    # Save checkpoint every 100k steps
    checkpoint_cb = CheckpointCallback(
        save_freq=100_000,
        save_path="./models/",
        name_prefix="champbot"
    )

    print("Training started! Press Ctrl+C to stop.")
    print("Models saved to ./models/  every 100k steps")
    print("-" * 50)

    try:
        model.learn(
            total_timesteps=5_000_000,
            callback=checkpoint_cb,
            progress_bar=True
        )
    except KeyboardInterrupt:
        print("\nTraining stopped.")

    model.save("models/champbot_final")
    print("Saved final model to models/champbot_final.zip")