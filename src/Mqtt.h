
/* 
mqtt class header file
*/

#pragma once
#include "PubSubClient.h"
#include "platform.h"

class Gpio; // forward declaration - see Gpio.h; only a pointer is needed here

class Mqtt {
    private:
        PubSubClient client;

        char* nodeName;          // Pointer to node name used for identifying self in MQTT connection
        char* deviceId;          // Pointer to device id (string)
        unsigned long intervlHb; // Heartbeat interval --
        char* topicPrepend;      // Prepend to (almost all) topics
        boolean _initialized = false; // true once begin() has actually run
        Gpio* _gpio = nullptr;   // set via setGpio() - lets "gpio <name> on/off" commands reach it
        char* _serverIp = nullptr; // owned copy of an explicitly-provided server IP, if any

    public:
        void setGpio(Gpio* gpio){ this->_gpio = gpio; }
        boolean connected=false; // True is connected, false if not
        boolean isInitialized(){ return _initialized; } // true once begin() has run (heartbeat is active)
        Mqtt(void);
        // Node name, device id, and (optionally) an explicit server IP -
        // falls back to the hardcoded default if serverIp is null/empty.
        // A copy of serverIp is taken internally (PubSubClient::setServer
        // only stores the pointer it's given, so the caller's copy - e.g.
        // a temporary String - doesn't need to outlive this call).
        void begin(char * nodeName, char * deviceId, const char * serverIp = nullptr);
        boolean connect();       // Attempt to conneect to mqtt broker
        void subscribe();        // subscribe subject with callback
        void publish(const char *, const char *); // publish to a subject

        void publishHello();    // publish to a subject
        void publishHeartbeat();    // regular heartbeat
        void unsubscribe();      // this must be possible! 

        void handleCallback(char *, byte *, unsigned int);   // route callback by subject
        void handle();           // generic handle function for parent run-loop

};
