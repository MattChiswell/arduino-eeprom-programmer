# Arduino EEPROM Programmer

### EEPROM Programmer written for Arduino using Direct Register Access

This code was originally based on the concept demonstrated by [Ben Eater](https://eater.net/) with his [EEPROM Programmer](https://github.com/beneater/eeprom-programmer). I started modifying the code and ended up re-writing the vast majority of it, moving away from the higher-level Arduino library functions and into Direct Register access instead.

This was written for and tested with ATMega4809 boards, it will not function on any board <= ATMega328 as the entire register interaction layer has changed significantly with the 4809 series. Using the in-built emulator packaged with the Arduino IDE you should be able to write code that conforms to the 328 series PORT registers on the newer 4809 chips, but unfortunately I ran into issues when doing so (on a seperate project, I found the timers completely unusable using the 328 register mapping under the emulation).

There will be a circuit schematic posted showing the pin mappings that are assumed in the code. The circuit is almost identical to the one designed by [Ben Eater](https://github.com/beneater/eeprom-programmer/blob/master/schematic.png).

*Note: the project was originally compiled outside of the Arduino IDE, as `main.cpp` with the required includes. This may not be as accessible so I have provided a .ino file that makes use of the default `setup()` and `loop()` functions.*
