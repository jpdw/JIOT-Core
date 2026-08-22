#include <Arduino.h>

#include "buildConfig.h"
#include "coreDebug.h"
#include "Core.h"
#ifdef INCLUDE_DEBUG
  #include <ESPTelnet.h>
#endif



#define FREEHEAP_REPORT                    \
    SER.print("Freeheap reduction = "); \
    SER.println(before - ESP.getFreeHeap());
#define FREEHEAP_BASELINE uint32_t before = ESP.getFreeHeap();
/*
    FREEHEAP_BASELINE
    FREEHEAP_REPORT
*/

Core *core;

void setup()
{
    // put your setup code here, to run once:

    core = new Core();
    core->start();

    // Exercise Core's new Gpio helper - register D5/D6/D7 as named outputs
    // so loop() can toggle them as a simple hardware sanity check.
    // Guarded on the real board macro, not on D5/D6/D7 themselves: on this
    // core version they're typed constants, not #define macros, so
    // `#if defined(D5)` silently evaluates false everywhere (verified via
    // -v build output) rather than actually detecting the board.
#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
    core->gpio.configureOutput("D5", D5);
    core->gpio.configureOutput("D6", D6);
    core->gpio.configureOutput("D7", D7);
#endif

    // initialise serial data receiption/processing

    // register callback for simulated receipt of serial data
}


#define RATE_LIMIT_MAX_S 120
boolean rate_limited(float value){
    static float valuePrevious = 0;     /* last value chosen to send */
    static unsigned long timeLatestNext = 0;   /* time of last send */
    unsigned long now = millis();

    if((now > timeLatestNext) || (value != valuePrevious)){
        valuePrevious =  value;
        timeLatestNext = now + (RATE_LIMIT_MAX_S * 1000);
        return true;
    }else{
        return false;
    }
}


char rawMqtt[170];

#define INTERVAL 10000

void loop()
{
    static unsigned long next_run_millis, millis_now;
    static boolean has_run = false;

    core->handle();

    // handle serial reception/processing...

    millis_now = millis();
    if (millis_now > next_run_millis)
    {
        next_run_millis = millis_now + INTERVAL;

        SER.print(">>> Freeheap = ");
        SER.println(ESP.getFreeHeap());

#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
        static boolean gpioTestState = false;
        gpioTestState = !gpioTestState;
        core->gpio.setOutput("D5", gpioTestState);
        core->gpio.setOutput("D6", gpioTestState);
        core->gpio.setOutput("D7", gpioTestState);
        SER.print(">>> GPIO test toggle (D5/D6/D7) = ");
        SER.println(gpioTestState ? "ON" : "OFF");
#endif

        if (!has_run)
        {
            has_run = true;

            // Run something once
        }
    }

}
