#!/usr/bin/env pybricks-micropython
from pybricks.ev3devices import Motor, UltrasonicSensor
from pybricks.parameters import Port
from pybricks.robotics import DriveBase
from pybricks.tools import wait, StopWatch

eyes = UltrasonicSensor(Port.S4)
left = Motor(Port.B)
right = Motor(Port.C)
robot = DriveBase(left, right, wheel_diameter=56, axle_track=114)

timer = StopWatch()
log = []

while timer.time() < 10000:          # 10 секунд
    d = eyes.distance()
    if d > 200:
        robot.drive(150, 0)
    else:
        robot.drive(-150, 0)
    log.append((timer.time(), d))
    wait(50)

robot.stop()

with open("attempt1.csv", "w") as f:
    for t, d in log:
        f.write("{},{}\n".format(t, d))
for t, d in log:
    print(t, d)
