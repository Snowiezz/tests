import inspect
from rlbot.parsing.rlbot_config_parser import create_bot_config_layout

config = create_bot_config_layout()
config.parse_file(r'C:\Users\domsa\Desktop\tests\rocket_league_bot\rlbot_cfg\match.cfg', 2)

# Print all headers and keys that were registered
for header_name, header in config.headers.items():
    print(f'HEADER: {repr(header_name)}')
    for key_name, key in header.values.items():
        print(f'  KEY: {repr(key_name)}')