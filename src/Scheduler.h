/*
    Scheduler - minimal non-blocking, millis()-based cooperative scheduler

    Lets an application register a periodic or one-shot callback under a
    fixed set of slots, then relies on Core::handle() to poll them each
    loop iteration. Deliberately simple (no dynamic allocation, no
    dependencies beyond Arduino's millis()) so it fits Core's existing
    "nothing in this library blocks" design - see Mqtt's non-blocking
    connect/retry logic for the same pattern.

    Superseded a previous, disabled scheduler.cpp.txt/h.txt that was tied
    to a different, older project (hardcoded external callbacks like
    flash_led/radio_loop that don't exist here) - this is a clean rewrite,
    not a re-enable of that code.
*/
#pragma once

#include <Arduino.h>

#define SCHEDULER_MAX_SLOTS 8

class Scheduler {
    public:
        // Registers callback to run every intervalMs (recurring=true), or
        // once after intervalMs (recurring=false). Returns the slot id
        // (>= 0) to later cancel(), or -1 if every slot is in use.
        int schedule(unsigned long intervalMs, void (*callback)(), boolean recurring = true);
        void cancel(int slot); // stop and free a slot; safe to call with -1 or an already-free slot
        void handle();          // call once per Core::handle() loop

    private:
        struct Slot {
            boolean active = false;
            boolean recurring = true;
            unsigned long interval = 0;
            unsigned long nextEvent = 0;
            void (*callback)() = nullptr;
        };
        Slot _slots[SCHEDULER_MAX_SLOTS];
};
