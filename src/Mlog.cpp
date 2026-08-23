// include this for logging (production logging) and debugging (no production)
//

#define mlVerbose 5 
#define mlDebug 4
#define mlInfo 3
#define mlWarning 2
#define mlErrors 1

#include "buildConfig.h"

#include <Arduino.h>
#include "Mlog.h"

#include <PubSubClient.h> // need mqtt header
#include "wlan.h"         // defines ConnectedStatus enum
//#include "SoftwareSerial.h"


//#if DEBUG_USE_SOFTSERIAL == 1
//extern SoftwareSerial swSer1;
//#endif

#ifdef INCLUDE_DEBUG
  ESPTelnet Telnet;
#endif

//#include "globals.h"

//
// Mlog - log to mqtt
//
// Send log message (to mqtt) - allowing the node to log events remotely
// in a 'syslog' type way (albeit to mqtt)
//
// Mlog requires a String contain the log message to be sent
//
// Mlog also sends the message to serial/remote console for debugging purposes
// is flags for this are enabled.
//

// Bare subtopic - Mqtt::publish() already prepends "<TOPIC_CONTEXT>/<deviceId>/"
// to whatever it's given (see Mqtt::publishHeartbeat() for the same pattern).
// This used to be a fully-qualified "device/%s/log" string built per-instance
// via malloc/sprintf, which meant it got double-prepended into
// "home/<id>/device/<id>/log" once passed through publish() - "device/<id>/..."
// is reserved for the special startup-only hello/cmnd topics (see
// buildConfig.h's documented scheme), not logging. Fixed for every instance,
// so no per-instance allocation is needed any more.
const char *mqttLogSubtopic = "log";

Mlog::Mlog(void){
    this->generalBuffer = new char[48];
}

void Mlog::begin(char * deviceIdPtr){
    this->begin(deviceIdPtr,0);
}

void Mlog::startRemoteDebug(){
    // Initialize telnet-based remote debug output
#ifdef INCLUDE_DEBUG
	Telnet.begin(); // Start telnet server on the default port (23)

    this->log("Telnet debug server started");
#endif
    // End off setup
}

void Mlog::begin(char * deviceIdPtr, Mqtt * mqttClientPtr){
    this->deviceId = deviceIdPtr;
    this->setMqttClient(mqttClientPtr);

    // Send an initial log
    sprintf(generalBuffer,"%s - Starting mLog", this->deviceId);
    this->log(generalBuffer);
}

void Mlog::setMqttClient(Mqtt * mqttClientPtr){
    this->mqttClient = mqttClientPtr;
}


void Mlog::log(const char *msg)
{
    char *msgBuffer = NULL;
    //#ifdef INCLUDE_DEBUG
    //  if((state == CONNECTED)||(enableDebug)){
    //#else
    //  if(state == CONNECTED){
    //#endif
    const char *msgTemplate = "[%d] %s";
    msgBuffer = (char *)malloc(strlen(msg) + 14);
    sprintf(msgBuffer, msgTemplate, millis(), msg);

    // MQTT publish and local serial output are this class's core purpose
    // (a "syslog"-style log, per the file header comment) - not a debug-
    // only feature, so both happen unconditionally. Telnet remote debug
    // stays opt-in behind INCLUDE_DEBUG: it's an unauthenticated network
    // listener, unlike local serial or a normal MQTT publish.
    if (this->mqttClient){
        this->mqttClient->publish(mqttLogSubtopic, msgBuffer);
    }

    SER.println(msgBuffer);

    #ifdef INCLUDE_DEBUG
        Telnet.println(msgBuffer);
    #endif
    if (msgBuffer)
    {
        free(msgBuffer);
    }
}

// Overloaded variant to support use of Arduino String
void Mlog::log(String msg)
{
    this->log(msg.c_str());
}

// Overloadedd variant to support setting message levels
void Mlog::log(Level msgLevel, String msg)
{
    if(msgLevel <= this->level){
        this->log(msg.c_str());
    }
}

void Mlog::setLevel(Level newLevel)
{
    this->level = newLevel;
}
/*
void Mlog::log(char *str, ...)
{
  int i, count=0, j=0, flag=0;
  char temp[ARDBUFFER+1];
  for(i=0; str[i]!='\0';i++)  if(str[i]=='%')  count++;

  va_list argv;
  va_start(argv, count);
  for(i=0,j=0; str[i]!='\0';i++)
  {
    if(str[i]=='%')
    {
      temp[j] = '\0';
      Serial.print(temp);
      j=0;
      temp[0] = '\0';

      switch(str[++i])
      {
        case 'd': Serial.print(va_arg(argv, int));
                  break;
        case 'l': Serial.print(va_arg(argv, long));
                  break;
        case 'f': Serial.print(va_arg(argv, double));
                  break;
        case 'c': Serial.print((char)va_arg(argv, int));
                  break;
        case 's': Serial.print(va_arg(argv, char *));
                  break;
        default:  ;
      };
    }
    else 
    {
      temp[j] = str[i];
      j = (j+1)%ARDBUFFER;
      if(j==0) 
      {
        temp[ARDBUFFER] = '\0';
        Serial.print(temp);
        temp[0]='\0';
      }
    }
  };
  Serial.println();
  return count + 1;
}
*/


void Mlog::handle(){
#ifdef INCLUDE_DEBUG
    Telnet.loop();
#endif
}