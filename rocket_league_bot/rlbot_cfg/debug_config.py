from rlbot.parsing.bot_config_bundle import PARTICIPANT_CONFIG_KEY, PARTICIPANT_CONFIGURATION_HEADER
from rlbot.parsing.rlbot_config_parser import create_bot_config_layout

config = create_bot_config_layout()
config.parse_file(r'C:\Users\domsa\Desktop\tests\rocket_league_bot\rlbot_cfg\match.cfg', 2)
val = config.get(PARTICIPANT_CONFIGURATION_HEADER, PARTICIPANT_CONFIG_KEY, 0)
print('VALUE:', repr(val))