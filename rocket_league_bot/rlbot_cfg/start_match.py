from rlbot.setup_manager import SetupManager
from rlbot.matchcomms.server import launch_matchcomms_server
import time

if __name__ == '__main__':
    sm = SetupManager()
    print("Loading config...", flush=True)
    sm.load_config(config_location=r'C:\Users\domsa\Desktop\tests\rocket_league_bot\rlbot_cfg\match.cfg')
    print("Connecting to game...", flush=True)
    sm.connect_to_game()
    print("Connected! Waiting 5 seconds for RL to settle...", flush=True)
    time.sleep(5)
    print("Starting match...", flush=True)
    sm.start_match()
    print("Launching bot processes...", flush=True)
    sm.matchcomms_server = launch_matchcomms_server()
    sm.launch_bot_processes()
    print("Match started! Running loop...", flush=True)
    sm.infinite_loop()