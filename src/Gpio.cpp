#include "Gpio.h"
#include <string.h>

int Gpio::findByName(const char* name){
    for(int i = 0; i < GPIO_MAX_PINS; i++){
        if(_entries[i].inUse && strcmp(_entries[i].name, name) == 0){
            return i;
        }
    }
    return -1;
}

int Gpio::addEntry(const char* name, uint8_t pin, boolean isOutput, boolean inverted){
    // Already registered under this name? Reconfigure that slot rather than
    // consuming another one.
    int idx = findByName(name);
    if(idx == -1){
        for(int i = 0; i < GPIO_MAX_PINS; i++){
            if(!_entries[i].inUse){
                idx = i;
                break;
            }
        }
    }
    if(idx == -1){
        return -1; // no free slots
    }
    _entries[idx].name = name;
    _entries[idx].pin = pin;
    _entries[idx].isOutput = isOutput;
    _entries[idx].inverted = inverted;
    _entries[idx].inUse = true;
    return idx;
}

boolean Gpio::configureOutput(const char* name, uint8_t pin, boolean inverted){
    if(addEntry(name, pin, true, inverted) == -1){
        return false;
    }
    pinMode(pin, OUTPUT);
    return true;
}

boolean Gpio::configureInput(const char* name, uint8_t pin, boolean inverted){
    if(addEntry(name, pin, false, inverted) == -1){
        return false;
    }
    pinMode(pin, INPUT);
    return true;
}

boolean Gpio::setOutput(const char* name, boolean state){
    int idx = findByName(name);
    if(idx == -1 || !_entries[idx].isOutput){
        return false;
    }
    boolean physicalHigh = _entries[idx].inverted ? !state : state;
    digitalWrite(_entries[idx].pin, physicalHigh ? HIGH : LOW);
    return true;
}

int Gpio::readInput(const char* name){
    int idx = findByName(name);
    if(idx == -1 || _entries[idx].isOutput){
        return -1;
    }
    int raw = digitalRead(_entries[idx].pin);
    if(_entries[idx].inverted){
        return (raw == HIGH) ? LOW : HIGH;
    }
    return raw;
}
