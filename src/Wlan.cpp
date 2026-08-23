/*
  ESP8266 Lighting Controller by Jon Wilkins
  
  WLAN Config & set-up

*/

#include "Wlan.h"
#include "buildConfig.h"
#include <EEPROM.h> // only for Wlan::migrateLegacyEeprom() - see Wlan.h

boolean settingMode;
WiFiClient wifiClient;
String ssidList;
DNSServer dnsServer;
WebServerClass webServer(80);

// Constants
#define WLAN_ASSOCIATION_TIMEOUT 15 /* seconds */
// Bytes per legacy EEPROM-format profile: 32 SSID + 64 password + 4 MQTT
// IP = 100. Only relevant to Wlan::migrateLegacyEeprom() now - profiles
// are stored as JSON on LittleFS (WLAN_PROFILES_FILE) for everything else.
#define WLAN_PROFILE_EEPROM_SIZE 100

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
    // Ensure case softAP is disabled
    WiFi.softAPdisconnect(true);

    if (this->readConfig())
    {
        WiFi.mode(WIFI_STA);

        // Try every remembered profile that actually has a saved SSID, in
        // slot order, until one succeeds or we run out - lets a device
        // move between several known networks (dev/test, home, off-grid,
        // ...) without needing to guess which one it's on.
        for (unsigned int i = 0; i < MAX_WLAN_PROFILES; ++i)
        {
            if (wlanConfig[i].ssid.length() == 0)
            {
                continue; // empty slot
            }

            if (this->wlanAssociate(i) == true)
            {
                SER.print("connected using profile #");
                SER.println(i + 1); // printed 1-based for humans
                this->profileIndex = i;
                break;
            }
        }

        if (this->state != WLAN_STA_CONNECTED)
        {
            SER.println("failed to connect using any saved profile");
        }
    }

    // If not connected, start the set-up mode AP
    if (this->state != WLAN_STA_CONNECTED){
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

String Wlan::getMqttIp()
{
    // Only meaningful once state == WLAN_STA_CONNECTED (profileIndex is set
    // at that point in begin()); guard the bound regardless since this may
    // be called before/without that ever happening.
    if (this->profileIndex < MAX_WLAN_PROFILES)
    {
        return this->wlanConfig[this->profileIndex].ipaddr;
    }
    return "";
}

/*
 *  migrateLegacyEeprom
 *
 *  One-time migration from the pre-LittleFS storage format (issue #7):
 *  reads whatever profiles are present in raw EEPROM, using the exact
 *  byte layout that used to be Wlan's primary storage, into wlanConfig[].
 *  Only called from readConfig() when WLAN_PROFILES_FILE doesn't exist
 *  yet - once migrated, this code path is never exercised again on that
 *  device (EEPROM itself is left untouched; simply not read again).
 *
 *  Returns true if at least one profile was found and migrated.
 */
boolean Wlan::migrateLegacyEeprom()
{
    EEPROM.begin(512);

    boolean found = false;

    SER.print("Checking for legacy EEPROM profile data... ");
    if (!(EEPROM.read(0) == 0xAA && EEPROM.read(1) == 0x55 && EEPROM.read(2) == 0xAA))
    {
        SER.println("none found");
        return false;
    }
    SER.println("found - migrating to " WLAN_PROFILES_FILE);

    for (unsigned int index = 0; index < MAX_WLAN_PROFILES; ++index)
    {
        unsigned int offset = 4 + (index * WLAN_PROFILE_EEPROM_SIZE);

        wlanConfig[index].ssid = "";
        wlanConfig[index].pass = "";
        wlanConfig[index].ipaddr = "";

        if (EEPROM.read(offset) == 0 || EEPROM.read(offset) == 255)
        {
            continue; // empty slot
        }

        for (int i = 0; i < 32; ++i)
        {
            wlanConfig[index].ssid += char(EEPROM.read(offset + i));
        }
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

        SER.print("Migrated profile #");
        SER.print(index + 1); // printed 1-based for humans
        SER.print(": SSID ");
        SER.println(wlanConfig[index].ssid);
        found = true;
    }

    return found;
}

/*
 *  saveProfiles
 *
 *  Writes every non-empty entry of wlanConfig[] to WLAN_PROFILES_FILE as
 *  JSON, overwriting the whole file - the in-RAM array is always the
 *  source of truth, this just flushes it.
 */
void Wlan::saveProfiles()
{
    JsonDocument doc;
    JsonArray profiles = doc["profiles"].to<JsonArray>();

    for (unsigned int i = 0; i < MAX_WLAN_PROFILES; ++i)
    {
        if (wlanConfig[i].ssid.length() == 0)
        {
            continue;
        }
        JsonObject profile = profiles.add<JsonObject>();
        profile["ssid"] = wlanConfig[i].ssid;
        profile["pass"] = wlanConfig[i].pass;
        profile["mqttIp"] = wlanConfig[i].ipaddr;
    }

    File f = LittleFS.open(WLAN_PROFILES_FILE, "w");
    if (!f)
    {
        SER.println("Failed to open " WLAN_PROFILES_FILE " for writing");
        return;
    }
    serializeJson(doc, f);
    f.close();
    SER.println("Saved profiles to " WLAN_PROFILES_FILE);
}

/*
 *  findProfileSlot
 *
 *  Returns the index of the existing profile matching `ssid` (so setup
 *  updates that slot in place - e.g. just changing the saved MQTT IP for
 *  a network you've already configured - rather than creating a
 *  duplicate), or the first empty slot for a new network, or -1 if every
 *  slot already holds a different network.
 */
int Wlan::findProfileSlot(String ssid)
{
    int firstEmpty = -1;
    for (unsigned int i = 0; i < MAX_WLAN_PROFILES; ++i)
    {
        if (wlanConfig[i].ssid == ssid)
        {
            return i;
        }
        if (firstEmpty == -1 && wlanConfig[i].ssid.length() == 0)
        {
            firstEmpty = i;
        }
    }
    return firstEmpty;
}

/*
  readConfig

  Load network config from LittleFS (WLAN_PROFILES_FILE). If that yields
  zero usable profiles - whether because the file doesn't exist, fails to
  parse, or exists but is unrelated content (e.g. leftover from other
  firmware previously run on the same hardware/flash - a real case hit
  during development, not just theoretical) - falls back to a one-time
  legacy-EEPROM migration attempt. A successful migration overwrites
  whatever was there with our own schema, so this is self-healing on the
  next boot either way. Returns
  - true if any profile was loaded (from JSON or freshly migrated)
  - false if no config is available (falls through to AP setup mode)
*/
boolean Wlan::readConfig()
{
    // format-and-retry rather than relying on each core's differing
    // begin() default (ESP8266 auto-formats on failure, ESP32 doesn't) -
    // see the storage-backend plan in issue #7 for why.
    if (!LittleFS.begin())
    {
        SER.println("LittleFS mount failed - formatting...");
        LittleFS.format();
        if (!LittleFS.begin())
        {
            SER.println("LittleFS still unavailable after format");
            return false;
        }
    }

    for (unsigned int i = 0; i < MAX_WLAN_PROFILES; ++i)
    {
        wlanConfig[i].ssid = "";
        wlanConfig[i].pass = "";
        wlanConfig[i].ipaddr = "";
    }

    unsigned int loaded = 0;

    if (LittleFS.exists(WLAN_PROFILES_FILE))
    {
        File f = LittleFS.open(WLAN_PROFILES_FILE, "r");
        if (!f)
        {
            SER.println("Failed to open " WLAN_PROFILES_FILE " for reading");
        }
        else
        {
            SER.print(WLAN_PROFILES_FILE " exists, size ");
            SER.print(f.size());
            SER.println(" bytes");

            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, f);
            f.close();
            if (err)
            {
                SER.print("Failed to parse " WLAN_PROFILES_FILE ": ");
                SER.println(err.c_str());
            }
            else
            {
                JsonArray profiles = doc["profiles"].as<JsonArray>();
                for (JsonObject profile : profiles)
                {
                    if (loaded >= MAX_WLAN_PROFILES)
                    {
                        break; // defensive - saveProfiles() never writes more than this
                    }
                    wlanConfig[loaded].ssid = profile["ssid"].as<String>();
                    wlanConfig[loaded].pass = profile["pass"].as<String>();
                    wlanConfig[loaded].ipaddr = profile["mqttIp"].as<String>();
                    SER.print("Loaded profile #");
                    SER.print(loaded + 1); // printed 1-based for humans
                    SER.print(": SSID ");
                    SER.println(wlanConfig[loaded].ssid);
                    ++loaded;
                }
            }
        }
    }
    else
    {
        SER.println("No " WLAN_PROFILES_FILE " yet");
    }

    if (loaded == 0 && migrateLegacyEeprom())
    {
        saveProfiles();
        return true;
    }

    return loaded > 0;
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
            SER.print(pass.length());
            SER.println(" chars (not logged)");
            String ipaddr_s = urlDecode(webServer.arg("ipaddr"));
            SER.print("MQTT IP Address: ");
            SER.println(ipaddr_s);

            // Update the existing slot for this SSID if it's already
            // saved, otherwise the first free slot - not always slot 0,
            // so multiple networks can be remembered at once.
            int slot = this->findProfileSlot(ssid);
            if (slot == -1)
            {
                String s = "<h1>Setup failed</h1><p>All ";
                s += MAX_WLAN_PROFILES;
                s += " saved network slots are already in use. Reset Wi-Fi settings first to free one up.</p>";
                webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
                return;
            }

            wlanConfig[slot].ssid = ssid;
            wlanConfig[slot].pass = pass;
            wlanConfig[slot].ipaddr = ipaddr_s;
            saveProfiles();

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
            LittleFS.remove(WLAN_PROFILES_FILE);
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




