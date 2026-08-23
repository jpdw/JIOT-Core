#include <Arduino.h>

#include "buildConfig.h"
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

    // Exercise Core's new Commands registry - MQTT "ping" (to device/<id>/cmnd
    // or device/all/cmnd) should trigger this. Demonstrates the extension
    // point an application (e.g. EFXC) uses to register its own commands,
    // alongside Core's own built-in "gpio"/"list".
    core->commands.add("ping", [](String payload){
        SER.println("pong");
    });

    // Exercise Core's new Gpio helper - register D5/D6/D7 as named outputs
    // so loop() can toggle them as a simple hardware sanity check.
    // Guarded on the real board macro, not on D5/D6/D7 themselves: on this
    // core version they're typed constants, not #define macros, so
    // `#if defined(D5)` silently evaluates false everywhere (verified via
    // -v build output) rather than actually detecting the board.
#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
    // Confirmed via live MQTT testing on a CB006 board: these lines are
    // active-low (all three HIGH = LED off), so mark them inverted.
    core->gpio.configureOutput("D5", D5, true);
    core->gpio.configureOutput("D6", D6, true);
    core->gpio.configureOutput("D7", D7, true);
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
        // Rotate one pin on at a time - OFF, D5, OFF, D6, OFF, D7, OFF, D5...
        // (10s/step) rather than driving all three simultaneously, to keep
        // power/heat down. Only runs while MQTT isn't connected - once it
        // is, manual "gpio <name> on/off" commands take over instead of
        // fighting with this auto-rotation every 10s.
        if (!core->mqtt.connected){
            static uint8_t gpioTestStep = 0;
            boolean d5on = (gpioTestStep == 1);
            boolean d6on = (gpioTestStep == 3);
            boolean d7on = (gpioTestStep == 5);
            core->gpio.setOutput("D5", d5on);
            core->gpio.setOutput("D6", d6on);
            core->gpio.setOutput("D7", d7on);
            SER.print(">>> GPIO test step = ");
            if(d5on) SER.println("D5");
            else if(d6on) SER.println("D6");
            else if(d7on) SER.println("D7");
            else SER.println("OFF");
            gpioTestStep = (gpioTestStep + 1) % 6;
        }
#endif

        if (!has_run)
        {
            has_run = true;

            // Run something once
        }
    }

}
