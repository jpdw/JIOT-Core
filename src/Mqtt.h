
/* 
mqtt class header file
*/

#pragma once
#include "PubSubClient.h"
#include "platform.h"


class Mqtt {
    private:
        PubSubClient client;
        
        char* nodeName;          // Pointer to node name used for identifying self in MQTT connection
        char* deviceId;          // Pointer to device id (string)
        unsigned long intervlHb; // Heartbeat interval -- 
        char* topicPrepend;      // Prepend to (almost all) topics

    public:
        boolean connected=false; // True is connected, false if not
        Mqtt(void);
        void begin(char *, char *); // Node name & device id
        boolean connect();       // Attempt to conneect to mqtt broker
        void subscribe();        // subscribe subject with callback
        void publish(const char *, const char *); // publish to a subject

        void publishHello();    // publish to a subject
        void publishHeartbeat();    // regular heartbeat
        void unsubscribe();      // this must be possible! 

        void handleCallback(char *, byte *, unsigned int);   // route callback by subject
        void handle();           // generic handle function for parent run-loop

};
