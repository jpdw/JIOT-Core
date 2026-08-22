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
        boolean configureOutput(const char* name, uint8_t pin); // register + pinMode(OUTPUT)
        boolean configureInput(const char* name, uint8_t pin);  // register + pinMode(INPUT)
        boolean setOutput(const char* name, boolean state);     // true = HIGH, false = LOW
        int readInput(const char* name);                        // returns HIGH/LOW, or -1 if unknown/not an input

    private:
        struct GpioEntry {
            const char* name = nullptr;
            uint8_t pin = 0;
            boolean isOutput = false;
            boolean inUse = false;
        };
        GpioEntry _entries[GPIO_MAX_PINS];

        int findByName(const char* name);
        int addEntry(const char* name, uint8_t pin, boolean isOutput);
};
