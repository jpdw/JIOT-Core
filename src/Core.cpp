/*
    Core library:
    - Wifi-based setup
    - OTA
    - MQTT 

    
 */

#include "Core.h"
#include "Mlog.h"
#include "coreDebug.h"
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

void Core::start(){

    // Set-up serial so we get debug early
    SER.begin(SERIAL_BIT_RATE);
    SER.begin(115200);

    // Get Device ID (string) early -- as this is fundamental
    this->deviceId = this->wlan.getDeviceIdSz();

    // Let MQTT-received "gpio <name> on/off" commands reach our own gpio
    // member - safe to wire up now regardless of WLAN/MQTT state, since
    // handleCallback() only ever fires once MQTT actually connects.
    mqtt.setGpio(&this->gpio);

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

    // Set-up storage & load configuration
    EEPROM.begin(512);



    // read hardware configuation
    // set-up hardware


    // set-up wifi/network
    wlan.begin();

    if(wlan.state == WLAN_STA_CONNECTED){
        SER.println("wlan.state == WLAN_STA_CONNECTED");

        #ifdef INCLUDE_DEBUG
        // Update logger with mqtt client details
        mlog.setMqttClient(&this->mqtt);
        mlog.startRemoteDebug();
        #endif

        // Start up other things that were dependant on being connected

    }
    
    switch((int)wlan.state){
        case 0:{
            SER.println("WLAN_STA_OFF");
            break;
        }
        case 1:{
            SER.println("WLAN_STA_STARTUP");
            break;
        }
        case 2:{
            SER.println("WLAN_STA_AP");
            break;
        }
        case 3:{
            SER.println("WLAN_STA_CONNECTING");
            break;
        }
        case 4:{
            SER.println("WLAN_STA_CONNECTED");
            break;
        }
    }

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
        mqtt.begin((char*)"new node", this->deviceId, savedMqttIp.c_str());
    }
    mlog.log("Setup complete");
}


void Core::handle(){
    wlan.handle();
    mqtt.handle();
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
        //mlog("OTA Push request started");
    });
    ArduinoOTA.onEnd([]() {
        //mlog("OTA Push request finished");
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
