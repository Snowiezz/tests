import RocketSim as rs
rs.init('collision_meshes')
arena = rs.Arena(rs.GameMode.SOCCAR)
car = arena.add_car(rs.Team.BLUE, rs.CarConfig(rs.CarConfig.OCTANE))
arena.step(8)
state = arena.get_gym_state()
print('Ball pos:', state[0])
print('All good!')