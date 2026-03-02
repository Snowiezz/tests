from rlbot.utils.class_importer import load_external_class
from rlbot.agents.base_agent import BaseAgent

cls, mod = load_external_class(r'C:\Users\domsa\Desktop\tests\rocket_league_bot\rlbot_cfg\agent.py', BaseAgent)
print('Class:', cls)
bot = cls(0, 0, 'ChampBot')
print('Instantiated:', bot)
bot.initialize_agent()
print('initialize_agent done')