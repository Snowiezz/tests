from rlbot.parsing.bot_config_bundle import PARTICIPANT_CONFIG_KEY, PARTICIPANT_CONFIGURATION_HEADER
from rlbot.parsing.rlbot_config_parser import create_bot_config_layout

config = create_bot_config_layout()
config.parse_file(r'C:\Users\domsa\Desktop\tests\rocket_league_bot\rlbot_cfg\match.cfg', 2)

val = config.get(PARTICIPANT_CONFIGURATION_HEADER, PARTICIPANT_CONFIG_KEY, 0)
print('VALUE index 0:', repr(val))
val2 = config.get(PARTICIPANT_CONFIGURATION_HEADER, PARTICIPANT_CONFIG_KEY, 1)
print('VALUE index 1:', repr(val2))

# Also print the raw file to confirm it saved correctly
with open(r'C:\Users\domsa\Desktop\tests\rocket_league_bot\rlbot_cfg\match.cfg', 'r') as f:
    print(f.read())