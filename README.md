# Stepper-Motor
Used an Arm Cortex STM32F microcontroller to code the operation of a stepper motor. Implemented *full stepping, half stepping, counter clowkise and clockwise rotation, and speed changeability*.

## Logic
- Pressing the onboard user button alternated between full step and half step
- Pressing the first external button triggers an interrupt to change between clockwise and counter clockwise roation
- Pressing the second external button triggers an interrupt to decrease the rotational speed
- Pressing the third external button triggers an interupt to increase the rotational speed
- Included safety features to ensure the motor is in a full step phase when switching from half stepping, a maximum and minimum speed to prevent any problems.
