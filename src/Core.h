/*
    Core library:
    - Wifi-based setup
    - OTA
    - MQTT 

    
 */

#pragma once
// Framework includes
#include <Arduino.h>
#include <EEPROM.h>
#include "platform.h"

// Library includes
#include "Wlan.h"
#include "Mqtt.h"

// Other config & info
#include "buildConfig.h"  
#include "build_info.h"
#include "coreDebug.h"

// Constants
#define SERIAL_BIT_RATE 115200

/*  Define "Core" class 
 */
class Core {
    char * deviceId;       // Pointer to device ID

public:
    void start();           // Start core features
    void handle();          // Loop
    Mqtt mqtt;              // Mqtt object
private:
    Wlan wlan;              // Wlan object
    
};

