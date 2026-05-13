# BMDLensController_PWM_LANC
FW for Bluepill on a carrier-board replacing FUJINON -BMD old analog servo control board.
Select Board: "Generic STM32F1 series" ;
Select Board part number: "Bluepill F103CB (or C8 with 128k)" ; 
Build with USB support set to "CDC (generic 'Serial' supersede U(S)ART)" ; 
This turns the Bluepill into a CDC-ACM, AT-command driven Fujinon -BMD lens controller
This requires a carrier board to interface with the lens potentiometer and motor: TODO

The project extends an old project: BMDLensController.
This new version:
- uses 3 hardware PWM to have more resolution and higher frequency
- adds a LANC port
- drops the capability to daisy-chain a Pan&Tilt controller
- adds the capability to use the UART (that was meant for Pan&Tilt daisy-chain) alongside the USB CDC to control the lens
The UART interface uses 8P1 at 9600bps.

The device is AT-command driven ; the main command to drive each servo is AT+X, were X is one of Z, I or F,
for, respectively, Zoom, Iris and Focus servo. Other commands exists to retrieve information about the HW/FW or state of every servo:
The command are case-insensitive (everything is upper case internally).
Due to limitation in the CDC implementation, answer from the previous command MUST be waited for before sending the next one.
Commands can be appended to each other by separating them with a semi-colon: ATZ1;&W is equivalent to sending ATZ1 and AT&W

ATI:
	get FW info (version and build date)

```text
ati
BMD Lens Controller v1.0.0, built on Jun 12 2024 at 08:44:15

OK
```

ATI1:
	get HW info (configured at build time for name, retrieved from memory for setpoints)


```text
ati1
FUJINON A17x9BMD-D24: Zoom in [9.0..153.0]mm, Iris in [1.9..16.0], Focus in [.9..999.9]m

OK
```

ATI2:
	get the setpoints for all servos

```text
ati2
ZOOM:
setPoints[ 0]={  9.0mm, 2640 steps}
setPoints[ 1]={ 20.0mm, 2146 steps}
setPoints[ 2]={ 40.0mm, 1853 steps}
setPoints[ 3]={ 80.0mm, 1584 steps}
setPoints[ 4]={153.0mm, 1320 steps}
parameters={.pwmScale=  4, .timeoutScale=100, .minSpeed=  1}
FOCUS:
setPoints[ 0]={   .9 m, 1323 steps}
setPoints[ 1]={  1.2 m, 1721 steps}
setPoints[ 2]={  1.5 m, 1961 steps}
setPoints[ 3]={  2.0 m, 2190 steps}
setPoints[ 4]={  3.0 m, 2414 steps}
setPoints[ 5]={  5.0 m, 2614 steps}
setPoints[ 6]={ 10.0 m, 2770 steps}
setPoints[ 7]={999.0 m, 2945 steps}
setPoints[ 8]={999.9 m, 2984 steps}
parameters={.pwmScale=  6, .timeoutScale= 64, .minSpeed=  4}
IRIS:
setPoints[ 0]={  1.9  , 3040 steps}
setPoints[ 1]={  2.8  , 2767 steps}
setPoints[ 2]={  4.0  , 2516 steps}
setPoints[ 3]={  5.6  , 2225 steps}
setPoints[ 4]={  8.0  , 1993 steps}
setPoints[ 5]={ 11.0  , 1786 steps}
setPoints[ 6]={ 16.0  , 1571 steps}
parameters={.pwmScale=  4, .timeoutScale= 32, .minSpeed=  2}

OK
```

The lens selected at build time will set the name and setpoints:
- the name has no real effect
- regarding the setpoints, your mileage may vary: they have been set for the lenses I could try, some of them have been fully disassembled and the values might be way off ; anyway they can be modified at runtime, and stored into Flash.

On startup, the servo will be loaded with the setpoints from the FW ; if some settings are present in the persistent storage they will be loaded and overwrite the values from the FW.
If you change the value in the FW after having copied them to Flash and want them to be applied, you need to force reloading the value from FW after they have been loaded from Flash.
This is done through ATZ:

ATZ:

Load the servo setpoints and settings from Flash

```text
atz

OK
```

ATZ1:

Load the servo setpoints and settings from FW

```text
atz1

OK
```

AT&W:

Store the value currently in memory into Flash

```text
AT&W

OK
```

If you want the value from the FW to be stored into Flash, use

ATZ1;&W

To retrieve the current ADC value for all servos, use:

```text
AT&v

at&v
ZOOM:[1711 .. 1712 .. 1714]
IRIS:[2223 .. 2225 .. 2226]
FOCUS:[2939 .. 2940 .. 2943]

OK
```

The command provides [minimum .. current .. maximum] values ; the minimum and maximum are reset to the current value each time a "move" is completed (more on that later).
The device uses 12-bit ADC and a low pass averaging filter to minimize the noise, but minimum and maximum are expected to be different. If they differ by more than 10, maybe something is wrong, though.

The "move" command is highly polymorphic ; there is 3 main ways of specifying a move:
- time and direction
- ADC value to reach, both relative and absolute
- setpoint
  
Each move command allow to specify the "speed" for the move (and the moves afterwards). You can even specify the speed without moving

Examples of "time and direction":

```text
AT+Z=+100m

OK
```

will move the Zoom servo in the forward direction for 100 milliseconds

```text
AT+Z=-1000m

OK
```

will move the Zoom servo in the backward direction for 1000 milliseconds

```text
AT+Z=+100000m,1

OK
```

will move the Zoom servo in the forward direction for 100 seconds, at the slowest possible speed.
Speeds range from 1 (slowest) to 16 (fastest). Each servo runs at maximum speed until setup otherwise.
However, the "minSpeed" setting for each servo limits the slowest speed actually allowed (to avoid getting stuck).

The "minSpeed" setting can be altered through the following syntax:

```text
at+z=m,4

OK
```

Now the minimum speed will not be lower than 4/16, whatever requested.

Examples of absolute ADC syntax:

```text
AT+z=2000

OK
```

will move, at the current speed setting, to the position where ADC value reaches 2000


```text
AT+Z=2000;+I=2000;+F=2000

OK
```

will move every servo, simultaneously, to reach the ADC value of 2000 ; each servo runs at its own pace, so the movements will not complete at the same time.


Examples of relative ADC move:

```text
AT+F=+20

OK
```

will change the position of the focus to reach current ADC + 20

AT+F=+1000;+Z=+60,1

will massively change focus at the current speed while also changing the zoom, at the slowest speed possible, by a small value.

For absolute as well as relative moves, the boundaries are checked: no move is allowed to end outside of the known setpoints for this servo.

The setpoint syntax has a dot '.' in the value (so 4 should be written 4. else it will be considered as an absolute ADC setting)
Examples of "setpoint" syntax

```text
AT+I=5.6

OK
```

will set Iris at F/5.6


```text
AT+Z=30.

OK
```

will set zoom to 30mmm


```text
AT+F=.9

OK
```

will set focus to 0.9m

As for all the other types of move, speed can be specified (else the current speed setting is used)

```text
AT+Z=128.,1

OK
```

will zoom to 128mm at the slowest allowed speed.

If the specified setpoint does no exists, but lies withing allowed values, the ADC setting will be linearly interpolated. This is for convenience only, and does not provide any kind of precision.
For example, if your lens has 15mm and 30mm zoom setting, you can use:

```text
AT+Z=20.

OK
```

but do not expect to reach the actual 20mm focal (ADC value are related to angular motion, not actual settings)

v3.0.1 introduces a new syntax for speed parameter: this is called timed moves. By adding an 's' after what was the PWM setting, it becomes the time the move should complete in.
The value is a float, and the timing can be precise down to millisecond, but don't expected miracle on small move or don't expect highly accurate timings.
Nonetheless, this allows for synchronized moves: 

AT+Z=48.,2.3s;+F=10.,2.3s

will reach 48mm zoom and 10m focus at the same time, in about 2.3s.

As part of a calibration process, you can set a new ADC value for any setpoint. The setpoint must exists, though.
Trying to set an ADC value for a non-existing setpoint results in an error.
You do that by either specifying the new value, of request using the current value:

```text
AT+F=5.6,,2222
FOCUS: setting 56 with provided adcValue 2222 instead of 2632 => 1

ERROR
```

Use provided ADC value:

```text
AT+I=5.6,,2222
IRIS: setting 56 with provided adcValue 2222 instead of 2225 => 0

OK
```

Use current ADC value:

```text
at+i=16.,,
IRIS: setting 160 with current adcValue 1796 instead of 1571 => 0

OK
```

Notice setpoints are internally stored as 10 times the value: 5.6 is internally 56 ; the value is an unsigned 16-bit value, so the range is 0.0 to 6553.5 inclusive.
This should be enough for any lens having realistic settings ; if you have a focal length beyond 6.5535 meters, this might be an issue.
For focus, the provided setpoints in the FW use the following conventions:
- setpoint are in meters (they often are written in both feet and meters on the lens)
- 999.0 is infinity (spot-on in the middle of the symbol)
- 999.9 is beyond infinity (the maximum reachable optical/mechanical position)
This has the poor side effect of rendering interpolated setpoints useless beyond the last actual setpoint (10m on the lenses I had access to) ; focus is meant to be driven in relative moves anyway (timed or ADC) or absolute values that have been already stored for a later use.
Once your are satisfied with your new setting(s), use AT&W to have them stored in Flash.

Each servo has 3 settings:
- pwmScale
- timeoutScale
- minSpeed

We have already seen minSpeed, but never acutally explained how it works.
"Speed" is obtained through driving the motor with a PWM signal ; the PWM goes from 1 over 16 (slowest possible speed) to 16 over 16 (full drive, maximum speed). Each servo has its own current-limiting power supply, a different motor, some gearbox / reduction and has to move more or less easy/smooth mechanical parts inside the lens. This has the side effect that with very low PWM setting, the drive mechanism can get stucked or not be smooth. To prevent this from happening, you can use minSpeed to forbid using too small a value ; using 16 will prevent any move to occur at less than the full speed.
The setting can be altered using the AT+x=M,PWM syntax:

```text
AT+Z=M,3

OK
```

will forbid Zoom servo to get lower than 3/16 PWM setting.

"timeoutScale" is used as a protection mechanism. At each move request, a timeout is computed. For the "duration" moves, this is the provided duration. For other move this boils down to delta ADC divided by speed.
When you ask for a new position, the FW will use the difference between the current ADC value and the ADC value to reach (apart from "duration" move, all moves are actually programmed as ADC values to reach).
This difference in ADC steps is multiplied by the provided timeoutScale and divided by the PWM setting applied at the start of the move (between 1 and 16, depending on the speed setting and the gap between the current position and the position to reach). The timeout value is in millisecond. Provided values range from 32 for iris to 100 for zoom. This is tradeoff: too low, the servo might stop before reaching the requested position, too high the servo might be "buzzing" for a while after reaching the requested position. Examples: a 1000-ADC step move with a 100 timeoutScale at full speed will have (1000 * 100) / 16 = 6250ms to complete ; a 1000-ADC step move with 100 as timeoutScale and the lowest speed will have (100*1000) / 1 = 100s to complete.
This pwmScale parameter is used to mimic the analog behavior of the original servo drive board: the closer to the target point we are, the slower we go (else we might overshoot and oscillate around the desired point).
Each time the FW "runs" a servo (every milliseconds) it compute the difference between the current ADC value and the target ADC value, this difference is then divided by pwmScale to get a PWM setting to apply (from 1 to 16).
When 0 is reached, the servo  is stopped (actually, the motor isn't driven anymore). Too big a value will prevent the requested setting to be reached (because the PWM setting will start dropping to 0 far away from the desired position) ; too low a value will trigger oscillations around the requested setting (because of inertia, if the requested value is reached at near full-speed, the servo will go beyond the desired point). Values between 4 and 6 do a good job. They are different for each servo because speed, inertia and friction are all different for each servo.

Altering the setting for timeoutScale is done with the following syntax:

```text
at+z=T,64

OK
```

Changing the pwmScale is done through:

```text
at+z=P,4

OK
```

Remember to use AT&W to record the value in Flash once they are OK.

# BMDLensController_PWM_LANC
