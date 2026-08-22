/*
    MQTT Wrapper library for Core
    - Paho MQTT
    - Helper wrapper class
    
 */

// Define the WLAN

#include "Mqtt.h"
#include "build_info.h"
#include "buildConfig.h"
#include "coreDebug.h"

#include "Mlog.h"

extern Mlog mlog;

#define HB_INTERVAL_S 60        // HB interval in seconds

const char *mqtt_server_ip = "10.1.1.33";
const unsigned int mqtt_server_port = 1883;

#define FREEHEAP_REPORT SER.print("Freeheap reduction = "); SER.println(before-ESP.getFreeHeap());
#define FREEHEAP_BASELINE uint32_t before = ESP.getFreeHeap();

/*
 *  Mqtt::begin()
 *
 *  Set up the MQTT client by connecting to the server and setting the callback
 *  functions - one of these will be 'on connect' which will then set up the
 *  subscriptions.
 *
 *  The method shown in the examples for this particular library rely upon using
 *  the reconnect method inside the main execution loop.  However, mqtt_start
 *  will make the first call to the reconnect method. *
 */



WiFiClient espClient;
PubSubClient mqttClient(espClient);

/*
client.setCallback(mqtt_callback);
where std::function<void(char*, uint8_t*, unsigned int)> callback
ie 
client.setCallback(std::function<void(char*, uint8_t*, unsigned int)> callback);
void mqtt_callback(char* topic, byte* payload, unsigned int length)
*/
void mqtt_callback(char* topic, byte* payload, unsigned int length){
    mlog.log("mqtt_callback -- not implemented");
}

Mqtt::Mqtt(void){
    this->intervlHb = HB_INTERVAL_S * 1000;
    this->client=mqttClient;
}

void Mqtt::begin(char * nodeName, char * deviceId){
    
    this->nodeName = nodeName;
    this->deviceId = deviceId;
    this->client.setServer(mqtt_server_ip, mqtt_server_port);
    
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    this->client.setCallback(std::bind( &Mqtt::handleCallback, this, _1,_2,_3));
    mlog.log("Mqtt::begin");
    
    //  Generate pre-pend to be used on (almost all) publishes
    this->topicPrepend = new char[strlen(TOPIC_CONTEXT) + strlen(this->deviceId) +3];
    sprintf(this->topicPrepend, "%s/%s/",TOPIC_CONTEXT, this->deviceId);

    // topicPrepend (and nodeName/deviceId above) are now safe to use -
    // nothing in handle()/publish() should touch them before this point
    this->_initialized = true;

    if(this->connect()){
        this->subscribe();
        // Publish hello to alert the network that this client has connected
        // (Note - should this differentiate between first-connection & reconnect?)
        this->publishHello();
    }
}

/*
 *  Mqtt::connect()
 *
 *  MQTT client has become disconnected for some reason -- attempt to
 *   reconnect with the MQTT broker.
 *
 */
boolean Mqtt::connect(){
    char client_id[50];
    snprintf(client_id, sizeof(client_id), "ESP8266 Client %s", nodeName);

    // Loop until we're reconnected
    while (!client.connected())
    {
        mlog.log("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect(client_id))
        {
            SER.print("connected as ");
            SER.println(client_id);
            // ... and subscribe to topics:

            this->connected = true;
            return true;
        }
        else
        {
            SER.print("failed, rc=");
            SER.print(client.state());
            SER.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(1000);
        }
    }
    return false;
}

void Mqtt::subscribe(){
    this->client.subscribe("home/cc/raw");
    this->client.subscribe("device/4EE184/log");
    mlog.log("Subscribed to topics");
}




/*
 *   prepend topic with '<context>/<device>/'
 */
void Mqtt::publish(const char * topic, const char * payload){
    char pubTopic[strlen(topic) + strlen(this->topicPrepend) + 1];
    snprintf(pubTopic, sizeof(pubTopic), "%s%s", this->topicPrepend, topic);
FREEHEAP_BASELINE   
    this->client.publish(pubTopic, payload);
FREEHEAP_REPORT 
}

void Mqtt::publishHeartbeat(){
    char msg[50];
    sprintf(msg,"{'freeheap':%zu}", ESP.getFreeHeap());
    SER.print("heartbeat = ");
    mlog.log(msg);
  
    this->publish("heaartbeat",msg);
}

void Mqtt::publishHello(){
  char msg[95];
  char topic[32];
  
  sprintf(msg,"{'core':'%s','bld':'%s','device':'%s','context':'%s','IP':'%s'}", \
    __BI__BUILD_NUMBER_STR,  __BI__DATEANDTIMESTAMP_STR, deviceId, TOPIC_CONTEXT, WiFi.localIP().toString().c_str());

  sprintf(topic,"%s/%s/hello", TOPIC_CONTEXT_INITIAL,deviceId);
  client.publish(topic,msg);

#ifdef INCLUDE_DEBUG
  //if(enableDebug){
    String a = (String)"[" + millis() + "] " + "mqtt: [" + topic + "] " + msg;
    debugV("%s",a.c_str());
  //}
#endif

}


extern char rawMqtt[170];

/*
 *  Mqtt::handleCallback
 *
 *  Callback function that will be called by the mqtt object on receipt
 *  of a message for a subscribed topic.  Function will either process
 *  the message or make onward calls, depending on the specific topic
 *
 */
void Mqtt::handleCallback(char *topic, byte *payload, unsigned int length){


    //mlog.log("Callback for ");
    //mlog.log(topic);  <--- this call appears to destroy the contents of the passed buffer..!

    SER.print("Callback for ");
    SER.println(topic);

    // Parse topic and dispatch appropriately

    if (strcmp(topic, "home/cc/raw")==0){
        // copy payload to shared buffer
        strcpy(rawMqtt, (char*)payload);
        return;
    }

    // Look for some simple topics
    if (strncmp(topic, "device/global/clock", 19) == 0)
    {
        // --->> mqtt_parse_message_clock(payload);
        return;
    }

    // Topic starts with "device/"?
    if (strncmp(topic, "device/", 7) == 0)
    {
        //Serial.print("match so far");
        char *lch = strchr(topic, '/');
        char *pch = strrchr(topic, '/');

        //bool global = false;
        if (strncmp(lch, "/global", pch - lch) == 0)
        {
            // global addressee
            //global = true;
        }

        // dispatch to handle payload
        // --->> command_received(payload, length, global);
    }


}

/*
 *  Mqtt::handle
 *
 *  Loop function to perform frequent checks/updates.
 *  Called by parent's 'handle' method.
 *  Performs, broadly, 2 sets of tasks:
 *   1 - tasks required only if connected to mqtt server
 *   2 - tasks required always
 */

void Mqtt::handle(){
    // begin() hasn't run yet (e.g. WLAN is still in AP/setup mode) - nothing
    // below is safe to touch until it has (topicPrepend/nodeName/deviceId
    // aren't set), so bail out entirely rather than crashing on first use.
    if (!this->_initialized){
        return;
    }

    static unsigned long nextHeartbeat = millis() + this->intervlHb;
    if (this->connected){
        this->client.loop();
    }


    // Send hearthbeat periodically
    if (millis() > nextHeartbeat){
        //Serial.println(".");
        nextHeartbeat = nextHeartbeat + this->intervlHb;
        this->publishHeartbeat();
    }
}


/* ==========================================================================
 * ==========================================================================
 *
 * MQTT RELATED FUNCTIONS
 *
 * ==========================================================================
 * ==========================================================================
 */

/*
 * parse_mqtt_message_clock
 *
 * Parse the payload then try
 */

bool mqtt_parse_message_clock(byte *payload)
{

    return true;
}



