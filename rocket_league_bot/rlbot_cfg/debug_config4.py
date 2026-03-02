import inspect
from rlbot.parsing.custom_config import ConfigObject

print(inspect.getsource(ConfigObject.parse_file))
print("---")
print(inspect.getsource(ConfigObject.get))