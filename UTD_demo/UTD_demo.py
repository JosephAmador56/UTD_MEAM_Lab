N_RUNS = 1

from north import NorthC9
from north import Scheduler
from Locator import *

c9 = NorthC9('A')
c9.default_vel = 50  # % speed #cts/s
c9.default_accel = 500000  #cts/s/s

c9.CAROUSEL_ROT = 4
c9.CAROUSEL_Z = 5

VACUUM_TOOL = 3

n9_pump = 0  #pump for n9 pipetting at address 0

pipette_n = -1
pipette_order = [i for i in range(2, 48, 3)] \
                + [i for i in range(1, 48, 3)] \
                + [i for i in range(0, 48, 3)]
def next_pipette():
    global pipette_n
    pipette_n += 1
    return pipette_order[pipette_n]

# z heights
safe_z = 288  #mm
vial_clamp_cap_h = 125  #mm
heater_clamp_cap_h = 126  #mm

def init_system():
    c9.home_robot()
    c9.home_pump(n9_pump)
    c9.home_axis(c9.CAROUSEL_ROT)
    c9.home_axis(c9.CAROUSEL_Z)

init_system()

for i in range(N_RUNS):
    #print('starting run ', i)
    #place first vial in heater clamp
    c9.goto_safe(home)
    c9.goto_safe(vial_rack[3*i+0])
    c9.close_gripper()
    c9.goto_safe(mixer[i])
    c9.uncap()
    h0 = c9.get_axis_position(3)
    c9.goto_safe(mixer[i+1])
    c9.open_gripper()
    c9.move_z(safe_z)

    #  should uncap and drop vial cap

    for j in [1, 2]:
        #place second vial in clamp and uncap
        c9.goto_safe(vial_rack[3*i+j])
        c9.close_gripper()
        c9.goto_safe(vial_clamp)
        c9.close_clamp()
        c9.uncap()
        h1 = c9.get_axis_position(3)
        c9.goto_safe(home)  # go to the home position (not the same as home operation)
        
        #demonstrate carousel movement
        c9.move_carousel(90, 80)  # 90*, 80mm down from top
        c9.delay(1)
        
        c9.set_pump_speed(1, 0)
        
        c9.set_pump_valve(1, c9.PUMP_VALVE_RIGHT)  # to reservoir
        c9.aspirate_ml(1, 4.5)  # pump 1, 4.5mL
        c9.set_pump_valve(1, c9.PUMP_VALVE_LEFT)
        c9.dispense_ml(1, 4.5) #pump 1, 4.5mL
        
        c9.move_carousel(135, 80)
        c9.set_pump_valve(2, c9.PUMP_VALVE_RIGHT)  # to reservoir
        c9.aspirate_ml(2, 0.5)  # pump 2, 0.5mL
        c9.set_pump_valve(2, c9.PUMP_VALVE_LEFT)
        c9.dispense_ml(2, 0.5) #pump 2, 0.5mL
        
        c9.move_carousel(0, 0)
        
        
        c9.goto_xy_safe(vial_clamp)
        c9.move_axis(3, h1)
        c9.cap()
        c9.open_clamp()
        c9.move_z(292)  #mm, max height
        c9.move_axis(0, 100000, vel=60, accel=10000)  # vortex mix with gripper
        c9.reduce_axis_position(0)  # reset axis 0 position to be within [0, 4000)
        
        # uncap again
        c9.goto_safe(vial_clamp)
        c9.close_clamp()
        c9.uncap()
        h1 = c9.get_axis_position(3)
        
        #grab a pipette and aspirate from second/third vial
        c9.goto_safe(p_rack[next_pipette()])
        c9.goto_safe(waypoint_0)
        c9.goto_safe(p_clamp)
        c9.aspirate_ml(n9_pump, 0.5)

        #dispense into first vial and remove pipette tip
        c9.goto_safe(waypoint_0)
        c9.goto_safe(p_mixer[i])
        c9.dispense_ml(n9_pump, 0.5)
        c9.goto_safe(p_remover_approach)
        c9.goto(p_remover)
        c9.move_z(292)
        
        #cap and replace second/third vial
        c9.goto_xy_safe(vial_clamp)
        c9.move_axis(3, h1)
        c9.cap()
        c9.open_clamp()
        c9.goto_safe(vial_rack[3*i+j])
        c9.open_gripper()
        
    c9.goto_safe(mixer[i+1])
    c9.close_gripper()
    c9.goto_xy_safe(mixer[i])
    c9.move_axis(3, h0)
    c9.cap()
    c9.open_gripper()
    

c9.move_z(safe_z)
c9.goto_safe(home)