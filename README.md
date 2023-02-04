# Arduino EEPROM Programmer

### EEPROM Programmer written for Arduino using Direct Registry Access

This code was originally based on the concept demonstrated by [Ben Eater](https://eater.net/) with his [EEPROM Programmer](https://github.com/beneater/eeprom-programmer). I started modifying the code and ended up re-writing the vast majority of it, moving away from the higher-level Arduino library functions and into Direct Registry access instead.

This was written for and tested with ATMega4809 boards, it will not function on any board <= ATMega328 as the entire register interaction layer has changed significantly with the 4809 series. Using the in-built emulator packaged with the Arduino IDE you should be able to write code that conforms to the 328 series PORT registers, but unfortunately I ran into issues when doing so.
