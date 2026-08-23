/*
    Core library:
    - Wifi-based setup
    - OTA
    - MQTT 

    
 */

#pragma once
// Framework includes
#include <Arduino.h>
#include "platform.h"

// Library includes
#include "Wlan.h"
#include "Mqtt.h"
#include "Gpio.h"
#include "Scheduler.h"

// Other config & info
#include "buildConfig.h"
#include "build_info.h"

// Constants
#define SERIAL_BIT_RATE 115200

/*  Define "Core" class 
 */
class Core {
    char * deviceId;       // Pointer to device ID
    const char * nodeName = "new node"; // MQTT node name; override via setNodeName() before start()

public:
    void start();           // Start core features
    void handle();          // Loop
    void setNodeName(const char * nodeName); // Must be called before start() to take effect
    const char * getDeviceId();  // Device ID string (valid once start() has run)
    WlanState getWlanState();    // Current WLAN connection state
    Mqtt mqtt;              // Mqtt object
    Gpio gpio;               // Named digital output/input helper
    Scheduler scheduler;    // Non-blocking task scheduler (call scheduler.schedule(...) any time after construction)
private:
    Wlan wlan;              // Wlan object

};

