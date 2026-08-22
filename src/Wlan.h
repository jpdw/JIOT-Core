/*
  ESP8266 Lighting Controller by Jon Wilkins
  
  WLAN Config & set-up

*/
#pragma once

#include <WiFiClient.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include "platform.h"

typedef enum WlanState 
{
    WLAN_OFF = 0, 
    WLAN_STARTUP = 1,
    WLAN_AP_MODE = 2, 
    WLAN_STA_CONNECTING = 3, 
    WLAN_STA_CONNECTED = 4
} WlanState_t;

class WlanScanNetworks{
    private:
        int networkCount = 0;
    public:
        int doScan();
        String getOptionList();
        String getArray();
};

class WlanConfig{

    public:
        String ssid = "";
        String pass = "";
        String ipaddr = "";
        //uint8_t ipaddr[4];
};

class Wlan{
    private:
        
    public:
        char * getDeviceIdSz();                         /* Return the device id as a string pointer */
        unsigned int getDeviceIdInt();                  /* Return the device id as a unint32 */
        boolean associated;                             /* Is associated with a WLAN? */
        WlanState state = WLAN_STARTUP;                 /* Is current WLAN state */
        unsigned int profileIndex;                      /* Index of the connected profile (or 0) */

        // Methods
        Wlan();                                         /* Constructor */
        boolean begin();                                /* Start WLAN */
        void handle();                                  /* Perform regular processing */
    private:
        char * deviceId;                                /* Pointer to device id string */
        unsigned int iDeviceId;                         /* uint32 representation of device id */
        WlanConfig wlanConfig[2];
        unsigned int wlanConfigCount = 0;        

        boolean wlanAssociate(unsigned int);
        boolean wlanAssociationRequest(const char *,const char *);
        boolean wlanCheckAssociation();

        boolean readConfig();
        boolean readWlanProfile(unsigned int);

        void setupMode();  
        String makePage(String, String);
        String urlDecode(String);
        String deviceID();
        void startWebServer();
        boolean restoreConfig();
        boolean readWlanConfig(unsigned int);
};



extern char * device_id;
extern WiFiClient wifiClient;
extern boolean settingMode;



// Constants
#define SETUP_WLAN_PREFIX "SETUP-"


