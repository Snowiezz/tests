from rlbot.agents.base_agent import BaseAgent, SimpleControllerState
from rlbot.utils.structures.game_data_struct import GameTickPacket
import subprocess, os, json, threading, queue

LOG = r'C:\Users\domsa\Desktop\tests\rocket_league_bot\rlbot_cfg\champbot.log'

def log(msg):
    with open(LOG, 'a') as f:
        f.write(msg + '\n')

class ChampBotAgent(BaseAgent):

    def initialize_agent(self):
        log("[ChampBot] initialize_agent called")
        exe = os.path.join(os.path.dirname(__file__),
              '..', 'build', 'Release', 'champ_bot_demo.exe')
        exe = os.path.abspath(exe)
        log(f"[ChampBot] Loading exe: {exe}")
        if not os.path.exists(exe):
            log(f"[ChampBot] ERROR: exe not found at {exe}")
            self.proc = None
            return
        self.proc = subprocess.Popen(
            [exe],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        log(f"[ChampBot] exe started, pid={self.proc.pid}")

        self.response_queue = queue.Queue()
        self.reader_thread = threading.Thread(target=self._read_stdout, daemon=True)
        self.reader_thread.start()
        log("[ChampBot] reader thread started")

    def _read_stdout(self):
        try:
            for line in self.proc.stdout:
                decoded = line.decode().strip()
                if decoded:
                    self.response_queue.put(decoded)
        except Exception as e:
            log(f"[ChampBot] reader thread exception: {e}")

    def get_output(self, packet: GameTickPacket) -> SimpleControllerState:
        ctrl = SimpleControllerState()
        if not hasattr(self, 'proc') or self.proc is None:
            return ctrl
        try:
            me   = packet.game_cars[self.index]
            ball = packet.game_ball

            # Find actual opponent: first car on opposite team
            opp = None
            for i in range(packet.num_cars):
                if i != self.index and packet.game_cars[i].team != me.team:
                    opp = packet.game_cars[i]
                    break
            if opp is None:
                opp = me  # fallback: no opponent found

            data = {
                "my_car": {
                    "x":         me.physics.location.x,
                    "y":         me.physics.location.y,
                    "z":         me.physics.location.z,
                    "vx":        me.physics.velocity.x,
                    "vy":        me.physics.velocity.y,
                    "vz":        me.physics.velocity.z,
                    "pitch":     me.physics.rotation.pitch,
                    "yaw":       me.physics.rotation.yaw,
                    "roll":      me.physics.rotation.roll,
                    "boost":     me.boost,
                    "on_ground": me.has_wheel_contact,
                    "team":      me.team
                },
                "opponent": {
                    "x":         opp.physics.location.x,
                    "y":         opp.physics.location.y,
                    "z":         opp.physics.location.z,
                    "vx":        opp.physics.velocity.x,
                    "vy":        opp.physics.velocity.y,
                    "vz":        opp.physics.velocity.z,
                    "boost":     opp.boost,
                    "on_ground": opp.has_wheel_contact,
                    "team":      opp.team
                },
                "ball": {
                    "x":  ball.physics.location.x,
                    "y":  ball.physics.location.y,
                    "z":  ball.physics.location.z,
                    "vx": ball.physics.velocity.x,
                    "vy": ball.physics.velocity.y,
                    "vz": ball.physics.velocity.z
                }
            }

            line = json.dumps(data) + "\n"
            self.proc.stdin.write(line.encode())
            self.proc.stdin.flush()

            try:
                resp = self.response_queue.get(timeout=0.1)
            except queue.Empty:
                log("[ChampBot] WARNING: no response within 0.1s")
                return ctrl

            out = json.loads(resp)
            ctrl.throttle  = out.get("throttle", 0)
            ctrl.steer     = out.get("steer", 0)
            ctrl.pitch     = out.get("pitch", 0)
            ctrl.yaw       = out.get("yaw", 0)
            ctrl.roll      = out.get("roll", 0)
            ctrl.boost     = out.get("boost", False)
            ctrl.jump      = out.get("jump", False)
            ctrl.handbrake = out.get("handbrake", False)

        except Exception as e:
            log(f"[ChampBot] EXCEPTION: {e}")

        return ctrl

    def retire(self):
        log("[ChampBot] retire called")
        if hasattr(self, 'proc') and self.proc:
            self.proc.terminate()