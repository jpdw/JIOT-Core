/*
  ESP8266 Lighting Controller by Jon Wilkins
  
  WLAN Config & set-up

*/

#include "Wlan.h"
#include "buildConfig.h"
#include "coreDebug.h"

boolean settingMode;
WiFiClient wifiClient;
String ssidList;
DNSServer dnsServer;
WebServerClass webServer(80);

// Constants
#define DEFAULT_PROFILE_INDEX 0 // 0-based - indexes directly into wlanConfig[2]
#define WLAN_ASSOCIATION_TIMEOUT 15 /* seconds */

// Define helper functions that are at the end of this file
void writeEeprom(unsigned int, String, String, const unsigned int*);
void showEeprom(int, int);


const IPAddress apIP(192, 168, 1, 1);

Wlan::Wlan(void)
{
    // Get Chip Id (from MAC)
    this->deviceId = new char[7];
    this->iDeviceId = platformGetChipId();
    sprintf(this->deviceId, "%06X", this->iDeviceId);
}

/*  Wlan::begin()
 * 
 *  Associaate with the WLAN if config exists. 
 *  Otherwise start the wlan setup process
 *  Start the built-in webserver
 * 
 *  Method returns true if successfully connected to a WLAN, false if not
 */
boolean Wlan::begin()
{
    unsigned int index = DEFAULT_PROFILE_INDEX; // Default profile index to use

    // Ensure case softAP is disabled
    WiFi.softAPdisconnect(true);

    if (this->readConfig())
    {
        WiFi.mode(WIFI_STA);
        //WiFi.hostname(string_buffer);
        //WiFi.begin(essid, password);

        if (this->wlanAssociate(index) == true)
        {
            SER.println("connected first time");
        }
        else
        {
            // Select the "other" profile (0-based: 0<->1)
            index = (index == 0) ? 1 : 0;

            if (this->wlanAssociate(index) == true)
            {
                SER.println("connected second time");
            }
            else
            {
                // failed twice
                SER.println("failed twice");
            }
        }
    }

    // If connected...
    if (this->state == WLAN_STA_CONNECTED){

        // Store the connected profile
        this->profileIndex=index;

    }else{
        // Start the set-up mode AP
        this->setupMode();
    }

    // Start the webserver

    startWebServer();

    return ((this->state == WLAN_STA_CONNECTED)?true:false);
}

char *Wlan::getDeviceIdSz()
{
    return deviceId;
}
unsigned int Wlan::getDeviceIdInt()
{
    return iDeviceId;
}

boolean Wlan::readWlanProfile(unsigned int index)
{
    // index is 0-based, matching wlanConfig[2] and readConfig()'s loop
    unsigned int offset = 4 + (index * 96);

    SER.print("Reading config profile #");
    SER.println(index + 1); // printed 1-based for humans

    this->wlanConfig[index].ssid = "";
    this->wlanConfig[index].pass = "";

    if (EEPROM.read(offset) != 0 && EEPROM.read(offset) != 255)
    {
        for (int i = 0; i < 32; ++i)
        {
            wlanConfig[index].ssid += char(EEPROM.read(offset + i));
        }
        SER.print("SSID: ");
        SER.println(wlanConfig[index].ssid);
        for (int i = 32; i < 96; ++i)
        {
            wlanConfig[index].pass += char(EEPROM.read(offset + i));
        }

        unsigned int a[4];
        for (int i = 0; i < 4; ++i)
        {
            a[i] = char(EEPROM.read(offset + 96 + i));
        }
        wlanConfig[index].ipaddr = String(a[0]) + "." + String(a[1]) + "." + String(a[2]) + "." + String(a[3]);

        return true;
    }
    else
    {
        return false;
    }
}

/*
  readConfig

  Read network config from EEPROM and returns
  - true if config read
  - false if no config read
*/
unsigned int eepromMapVersion = 0;
boolean Wlan::readConfig()
{
    SER.println("Reading EEPROM...");

    SER.print("Checking for EEPROM map... ");
    if (EEPROM.read(0) == 0xAA && EEPROM.read(1) == 0x55 && EEPROM.read(2) == 0xAA)
    {
        eepromMapVersion = EEPROM.read(3);
        SER.print("map version ");
        SER.println(eepromMapVersion);
    }
    else
    {
        SER.print("no map or unknown version");
        return false;
    }

    if (eepromMapVersion > 0)
    {

        for (int i = 0; i < 2; ++i)
        {
            readWlanProfile(i);
        }
    }
    else
    {
        return false;
    }

    return true;
}

/*
 *  wlanConnect 
 * 
 *  Starts the association attempt to the given WLAN
 * 
 *  ssid and password are specified in the arguements
 * 
 *  Function is non-blocking, returing the result
 *  of the call to WiFi.begin()
 */
boolean Wlan::wlanAssociationRequest(const char *ssid, const char *pass)
{
#ifdef DEBUG
    SER.println(" Trying to associate with ESSID...");
#endif
    return WiFi.begin(ssid, pass);
}

/*  
 *  wlanCheckConnection
 * 
 *  Blocking while WLAN connection is attempted, exiting on success
 *  on timeout after a defined number of seconds 
 *
 *  Returns true/false to indicate whether connection was successful
 */
boolean Wlan::wlanCheckAssociation()
{

    unsigned long millis_now = millis();
    static unsigned long millis_at_start = 0;
    static unsigned long timeout_at;
    int count = 0;

    // Get millis at this moment so we can be consistent with what is
    // the 'current' time
    millis_now = millis();

    // Record start of the connection attempt (we will block till its finished)
    if (millis_at_start == 0)
    {
        millis_at_start = millis_now;
        timeout_at = millis_at_start + (WLAN_ASSOCIATION_TIMEOUT * 1000);

        //Serial.print("Connection attempted start at ");
        //Serial.println(millis_at_start);
    }

    //Serial.print("Millis now ");
    //Serial.println(millis_now);

    while (1)
    {

        // Update current millis as this is the start of the blocking loop
        millis_now = millis();

        // Have we timed out?
        if (millis_now > timeout_at)
        {
            //Serial.println("Connection attempt duration has exceed timeout... exiting");
            // Reset millis_at_start in case we will re-call this process
            millis_at_start = 0;
            return false;
        }

        // Now check status of connection
        if (WiFi.status() == WL_CONNECTED)
        {
            SER.println();
            SER.println("WLAN connected!");

            SER.println("Connection time was:");
            SER.print(" start     : ");
            SER.println(millis_at_start);
            SER.print(" fimish    : ");
            SER.println(millis_now);
            SER.print(" time      : ");
            SER.println(millis_now - millis_at_start);
            SER.print(" iterations: ");
            SER.println(count);

            return true;
        }
        else
        {
            count++;
            yield();
        }
    }
}

/*  wlanAssociate
 *
 *  Begin an attempt to associate with the give SSID
 * 
 *  - Initiate the association request
 *  - Wait for assoication success or timeout
 * 
 *  Returns boolean on successul (true) or failure (false)
 */
boolean Wlan::wlanAssociate(unsigned int index)
{
    SER.print("Attempting to connect to index ");
    SER.println(index);

    this->state = WLAN_STA_CONNECTING;
    this->wlanAssociationRequest(wlanConfig[index].ssid.c_str(), wlanConfig[index].pass.c_str());

    // Blocking call to wait for (a) success or (b) timeout
    if (this->wlanCheckAssociation())
    {
        settingMode = false;
        this->startWebServer();

        this->state = WLAN_STA_CONNECTED;
        return true;
    }
    else
    {
        this->state = WLAN_STA_CONNECTING;
        return false;
    }
}



void Wlan::startWebServer()
{

    if (settingMode)
    {
        SER.print("IP Address: ");
        SER.println(WiFi.softAPIP());

        webServer.on("/settings", [&]() {
            String s = "<h1>Device Wi-Fi Settings</h1><p>";
            s += this->deviceId;
            s += "</p><p>Please select the ESSID from the scanned list and then enter the password.</p>";
            s += "<form method=\"get\" action=\"apsetup\"><label>ESSID: </label><select name=\"essid\">";
            s += ssidList;
            SER.println(ssidList);
            s += "</select><br>Password: <input name=\"pass\" length=64 type=\"password\"><br>MQTT IP Addr: <input name=\"ipaddr\" length=16 type=\"text\"><input type=\"submit\"></form>";
            webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
        });
        webServer.on("/apsetup", [&]() {
            String ssid = urlDecode(webServer.arg("essid")) + "\0";
            SER.print("ESSID: ");
            SER.println(ssid);
            String pass = urlDecode(webServer.arg("pass"));
            SER.print("Password: ");
            SER.println(pass);
            String ipaddr_s = urlDecode(webServer.arg("ipaddr"));
            SER.print("MQTT IP Address: ");
            SER.println(ipaddr_s);
            char ipaddr[16];
            unsigned int ip[4];
            ipaddr_s.toCharArray(ipaddr, 16);
            sscanf(ipaddr, "%u.%u.%u.%u", &ip[0], &ip[1], &ip[2], &ip[3]);

            SER.print(ip[0]);
            SER.print(".");
            SER.print(ip[1]);
            SER.print(".");
            SER.print(ip[2]);
            SER.print(".");
            SER.println(ip[3]);
            SER.print("done");

            writeEeprom(0, ssid, pass, ip);

            String s = "<h1>Setup complete.</h1><p>device will be connected to \"";
            s += ssid;
            s += "\" after the restart.";
            webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
            ESP.restart();
        });
        webServer.onNotFound([&]() {
            String s = "<h1>AP mode - " + String(this->deviceId) + "</h1><p><a href=\"/settings\">Wi-Fi Settings</a></p>";
            webServer.send(200, "text/html", makePage("AP mode - " + String(this->deviceId), s));
        });
    }
    else
    {
        SER.print("IP Address: ");
        SER.println(WiFi.localIP());
        webServer.on("/", [&]() {
            String s = "<h1>STA mode</h1><p>";
            s += this->deviceId;
            s += "</p><p><a href=\"/reset\">Reset Wi-Fi Settings</a></p>";
            webServer.send(200, "text/html", makePage("STA mode", s));
        });
        webServer.on("/reset", [&]() {
            for (int i = 0; i < 96; ++i)
            {
                EEPROM.write(i, 0);
            }
            EEPROM.commit();
            String s = "<h1>Wi-Fi settings have been reset.</h1><p>Please reboot device.</p>";
            webServer.send(200, "text/html", makePage("Reset Wi-Fi Settings", s));
        });
    };

    webServer.begin();
}



/*             */
/*  SETUP MODE */
/*             */

void Wlan::setupMode()
{
    char apSSID[33];

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    WlanScanNetworks scanNetworks;
    scanNetworks.doScan();
    ssidList = scanNetworks.getOptionList();

    delay(100);

    // Generate SSID
    sprintf(apSSID, "%s%s", SETUP_WLAN_PREFIX, this->deviceId); //SETUP_WLAN_PREFIX

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(apSSID);

    dnsServer.start(53, "*", apIP);

    SER.print("Starting Access Point at \"");
    SER.print(apSSID);
    SER.println("\"");

    settingMode = true;
    this->state = WLAN_AP_MODE;
}

String Wlan::makePage(String title, String contents)
{
    String s = "<!DOCTYPE html><html><head>";
    s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
    s += "<title>";
    s += title;
    s += "</title></head><body>";
    s += contents;
    s += "</body></html>";
    return s;
}

String Wlan::urlDecode(String input)
{
    String s = input;
    s.replace("%20", " ");
    s.replace("+", " ");
    s.replace("%21", "!");
    s.replace("%22", "\"");
    s.replace("%23", "#");
    s.replace("%24", "$");
    s.replace("%25", "%");
    s.replace("%26", "&");
    s.replace("%27", "\'");
    s.replace("%28", "(");
    s.replace("%29", ")");
    s.replace("%30", "*");
    s.replace("%31", "+");
    s.replace("%2C", ",");
    s.replace("%2E", ".");
    s.replace("%2F", "/");
    s.replace("%2C", ",");
    s.replace("%3A", ":");
    s.replace("%3A", ";");
    s.replace("%3C", "<");
    s.replace("%3D", "=");
    s.replace("%3E", ">");
    s.replace("%3F", "?");
    s.replace("%40", "@");
    s.replace("%5B", "[");
    s.replace("%5C", "\\");
    s.replace("%5D", "]");
    s.replace("%5E", "^");
    s.replace("%5F", "-");
    s.replace("%60", "`");
    return s;
}

/*
 *  Wlan::handle
 *
 *  Loop function to perform frequent checks/updates.
 *  Called by parent's 'handle' method.
 *  Performs, broadly, 3 sets of tasks:
 *   1 - tasks required only if wlan is associated as an STA
 *   2 - tasks required only if wlan is in AP/Setup mode
 *   2 - tasks required always
 */
void Wlan::handle()
{  
    // 1 - tasks when STA_CONNECTED
    if (this->state == WLAN_STA_CONNECTED){

    }
    // 2 - tasks in AP mode
    if(this->state == WLAN_AP_MODE){
        dnsServer.processNextRequest();
    }

    // 3 - tasks always
    webServer.handleClient();
}

/*
 * WlanScanNetworks class
 * 
 */
int WlanScanNetworks::doScan()
{
    int ret;
    ret = this->networkCount = WiFi.scanNetworks();
    SER.println("Found " + String(ret) + " networks");
    return ret;
}

String WlanScanNetworks::getOptionList()
{
    String optionList;
    for (int i = 0; i < this->networkCount; ++i)
    {
        optionList += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option>";
        SER.println(WiFi.SSID(i));
    }
    return optionList;
}


/*
 *  Non-class functions
 *  ===================
 */

void showEeprom(int addr_from, int length)
{
    SER.print("Reading EEPROM from ");
    SER.println(addr_from);
    SER.print(" for ");
    SER.println(length);
    SER.println(" bytes:");
    for (int i = 0; i < 12; ++i)
    {
        SER.print(addr_from + i);
        SER.print(": ");
        SER.print(EEPROM.read(addr_from + i));
        SER.print(" ");
       SER.println(char(EEPROM.read(addr_from + i)));
    }
}
void writeEeprom(unsigned int index, String ssid, String pass, const unsigned int mqttIp[4])
{
    unsigned int offset = 4 + (index * 96);
    unsigned int i;

    // Write signature
    EEPROM.write(0, 0xAA);
    EEPROM.write(1, 0x55);
    EEPROM.write(2, 0xAA);
    EEPROM.write(3, 0x01);

    // Clear EEPROM for this record/index
    for (int i = 0; i < 100; ++i)
    {
        EEPROM.write(offset + i, 0);
    }

    EEPROM.commit();

    SER.println("Writing ESSID to EEPROM...");

    for (i = 0; i < ssid.length(); i++)
    {
        EEPROM.write(offset + i, ssid[i]);
        //Serial.print("Addr ");
        //Serial.print(offset+i);
        //Serial.print(": ");
        //Serial.println(ssid[i]);
    }
    //Serial.print(offset+i);
    EEPROM.write(offset + i, 0);

    SER.println("Writing Password to EEPROM...");
    for (i = 0; i < pass.length(); ++i)
    {
        EEPROM.write(offset + 32 + i, pass[i]);
    }
    EEPROM.write(offset + 32 + i, 0);

    SER.println("Writing MQTT IP to EEPROM...");
    for (i = 0; i < 4; ++i)
    {
        EEPROM.write(offset + 96 + i, mqttIp[i]);
    }
    EEPROM.write(offset + 96 + i, 0);

    EEPROM.commit();
    SER.println("Write EEPROM done!");
}



