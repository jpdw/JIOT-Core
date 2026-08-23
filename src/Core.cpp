/*
    Core library:
    - Wifi-based setup
    - OTA
    - MQTT 

    
 */

#include "Core.h"
#include "Mlog.h"
#include <string.h>
#include <stdio.h>
//


#define INCLUDE_OTA_PUSH

#ifdef INCLUDE_OTA_PUSH
    #include <WiFiUdp.h>
    #include <ArduinoOTA.h>
#endif
#ifdef INCLUDE_OTA_PUSH
    bool enable_ota_push = true;
    void start_ota();
#endif

Mlog mlog;

const char* wlanStateName(WlanState state){
    switch((int)state){
        case WLAN_OFF: return "WLAN_OFF";
        case WLAN_STARTUP: return "WLAN_STARTUP";
        case WLAN_AP_MODE: return "WLAN_AP_MODE";
        case WLAN_STA_CONNECTING: return "WLAN_STA_CONNECTING";
        case WLAN_STA_CONNECTED: return "WLAN_STA_CONNECTED";
        default: return "UNKNOWN";
    }
}

void Core::setNodeName(const char * nodeName){
    this->nodeName = nodeName;
}

const char * Core::getDeviceId(){
    return this->deviceId;
}

WlanState Core::getWlanState(){
    return this->wlan.state;
}

void Core::start(){

    // Set-up serial so we get debug early
    SER.begin(SERIAL_BIT_RATE);

    // Get Device ID (string) early -- as this is fundamental
    this->deviceId = this->wlan.getDeviceIdSz();

    // Built-in commands, registered through the same mechanism an
    // application uses for its own (see Commands.h) - "gpio" replaces
    // what used to be hardcoded parsing inside Mqtt::handleCallback()
    // itself. Safe to wire up now regardless of WLAN/MQTT state, since
    // handleCallback() only ever fires once MQTT actually connects.
    this->commands.add("gpio", [this](String payload){
        char pinName[8];
        char state[8];
        if (sscanf(payload.c_str(), "%7s %7s", pinName, state) == 2){
            boolean on = (strcmp(state, "on") == 0);
            boolean off = (strcmp(state, "off") == 0);
            if (on || off){
                boolean ok = this->gpio.setOutput(pinName, on);
                SER.print("gpio command: ");
                SER.print(pinName);
                SER.print(" -> ");
                SER.print(on ? "ON" : "OFF");
                SER.println(ok ? " (applied)" : " (unknown pin)");
            }
        }
    });
    this->commands.add("list", [this](String payload){
        this->commands.list();
    });
    mqtt.setCommands(&this->commands);

    // Output minimal startup & build info
    SER.println();
    SER.print("Device ");
    SER.print(this->deviceId);
    SER.print("; Core build ");
    SER.print(__BI__BUILD_NUMBER);
    SER.print(" (");
    SER.print(__BI__DATEANDTIMESTAMP_STR);
    SER.println(")");

    mlog.begin(this->deviceId);

    //mlog.logf("a", "b");

    // read hardware configuation
    // set-up hardware


    // set-up wifi/network
    wlan.begin();

    if(wlan.state == WLAN_STA_CONNECTED){
        // MQTT-based logging is Mlog's core purpose, not a debug-only
        // feature - wire it up unconditionally. Telnet remote debug stays
        // opt-in behind INCLUDE_DEBUG (an unauthenticated network listener).
        mlog.setMqttClient(&this->mqtt);
        #ifdef INCLUDE_DEBUG
        mlog.startRemoteDebug();
        #endif

        // Start up other things that were dependant on being connected

    }

    // Reuses the same name mapping Core::handle()'s periodic status report
    // uses, rather than a second hand-duplicated switch (which had drifted
    // out of sync with the real WlanState names - e.g. printed
    // "WLAN_STA_OFF" for WLAN_OFF).
    SER.println(wlanStateName(wlan.state));

    if(wlan.state == WLAN_STA_CONNECTED){
        // Start OTA-Ardiono uplod
#ifdef INCLUDE_OTA_PUSH
        if(enable_ota_push){
            start_ota();
            mlog.log(Mlog::verbose,"OTA started");
        }
#endif
        delay(1000);
        String savedMqttIp = wlan.getMqttIp();
        mqtt.begin((char*)this->nodeName, this->deviceId, savedMqttIp.c_str());
    }
    mlog.log("Setup complete");
}


void Core::handle(){
    wlan.handle();
    mqtt.handle();
    scheduler.handle();
#ifdef INCLUDE_OTA_PUSH
    if(enable_ota_push){
        if(wlan.state == WLAN_STA_CONNECTED || wlan.state == WLAN_AP_MODE){
            ArduinoOTA.handle();
        }
    }
    mlog.handle();
#endif

    // Periodic status report - useful while no application layer is
    // exercising Core (e.g. examples/minimal.cpp)
    static unsigned long nextStatusReport = 0;
    if (millis() > nextStatusReport){
        nextStatusReport = millis() + 15000;
        SER.print(">>> Core status: WLAN=");
        SER.print(wlanStateName(wlan.state));
        SER.print(" MQTT initialized=");
        SER.print(mqtt.isInitialized() ? "yes" : "no");
        SER.print(" connected=");
        SER.println(mqtt.connected ? "yes" : "no");
    }
};

#ifdef INCLUDE_OTA_PUSH
void start_ota(){
    ArduinoOTA.onStart([]() {
        mlog.log("OTA Push request started");
    });
    ArduinoOTA.onEnd([]() {
        mlog.log("OTA Push request finished");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        SER.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        SER.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) SER.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) SER.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) SER.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) SER.println("Receive Failed");
        else if (error == OTA_END_ERROR) SER.println("End Failed");
    });
    ArduinoOTA.begin();
    //mlog("OTA Push support ready at " + WiFi.localIP().toString());
}
#endif
