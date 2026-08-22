/*
    Gpio - simple named digital output/input helper

    Lets an application register a pin under a short name, then set/read it
    by that name rather than juggling raw pin numbers everywhere. Deliberately
    minimal - Core provides the mechanism (name -> pin), not a device layout;
    the application decides which pins mean what for its own board.
*/
#pragma once

#include <Arduino.h>

#define GPIO_MAX_PINS 8

class Gpio {
    public:
        // inverted: if true, the physical signal is the opposite of what
        // the caller asks for (e.g. an active-low LED driver, so
        // setOutput(name, true) actually drives the pin LOW). Defaults to
        // false - the calling application opts in per pin, matching its
        // own board wiring, matching the same pattern already used by
        // EFXC's HwPort (see lib/Hal).
        boolean configureOutput(const char* name, uint8_t pin, boolean inverted = false); // register + pinMode(OUTPUT)
        boolean configureInput(const char* name, uint8_t pin, boolean inverted = false);  // register + pinMode(INPUT)
        boolean setOutput(const char* name, boolean state);     // true = "on" (HIGH unless inverted)
        int readInput(const char* name);                        // returns HIGH/LOW (post-inversion), or -1 if unknown/not an input

    private:
        struct GpioEntry {
            const char* name = nullptr;
            uint8_t pin = 0;
            boolean isOutput = false;
            boolean inverted = false;
            boolean inUse = false;
        };
        GpioEntry _entries[GPIO_MAX_PINS];

        int findByName(const char* name);
        int addEntry(const char* name, uint8_t pin, boolean isOutput, boolean inverted);
};
