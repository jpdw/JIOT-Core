#pragma once

#ifdef ESP8266
    #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>
    #include <ESP8266mDNS.h>
    typedef ESP8266WebServer WebServerClass;
#endif

#ifdef ESP32
    #include <WiFi.h>
    #include <WebServer.h>
    #include <ESPmDNS.h>
    typedef WebServer WebServerClass;
#endif

// Stable 6-hex-char chip ID, consistent with the convention already used in
// effects_controller/EFXC (folds ESP32's 48-bit MAC the same way they do).
uint32_t platformGetChipId();
