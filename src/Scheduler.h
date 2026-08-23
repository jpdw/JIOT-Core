/*
    Scheduler - minimal non-blocking, millis()-based cooperative scheduler

    Lets an application register a periodic or one-shot callback, then
    relies on Core::handle() to poll them each loop iteration. Fits Core's
    existing "nothing in this library blocks" design - see Mqtt's
    non-blocking connect/retry logic for the same pattern.

    Entries are allocated on demand (one `new` per schedule() call, freed
    on cancel() or when a one-shot fires) rather than drawn from a fixed
    array, and each carries an owner tag: Core itself schedules its own
    internal work as SCHEDULER_OWNER_CORE, while `core.scheduler` is also
    exposed for consuming modules to use directly (default
    SCHEDULER_OWNER_CLIENT) - a client-owned cancel() call cannot remove a
    Core-owned entry. Both requirements per JIOT-Core issue #3.

    Superseded a previous, disabled scheduler.cpp.txt/h.txt that was tied
    to a different, older project (hardcoded external callbacks like
    flash_led/radio_loop that don't exist here) - this is a clean rewrite,
    not a re-enable of that code.
*/
#pragma once

#include <Arduino.h>

enum SchedulerOwner {
    SCHEDULER_OWNER_CORE,
    SCHEDULER_OWNER_CLIENT
};

class Scheduler {
    public:
        ~Scheduler();

        // Registers callback to run every intervalMs (recurring=true), or
        // once after intervalMs (recurring=false). owner defaults to
        // SCHEDULER_OWNER_CLIENT; Core passes SCHEDULER_OWNER_CORE for its
        // own internal scheduling so client code can't cancel it. Returns
        // an id to later pass to cancel() - entries are allocated on
        // demand, there is no fixed limit.
        int schedule(unsigned long intervalMs, void (*callback)(), boolean recurring = true, SchedulerOwner owner = SCHEDULER_OWNER_CLIENT);

        // Cancels and frees entry `id`. A SCHEDULER_OWNER_CLIENT caller
        // (the default) cannot cancel a SCHEDULER_OWNER_CORE entry - only
        // Core itself (passing SCHEDULER_OWNER_CORE) can. Safe to call
        // with an unknown/already-cancelled id.
        void cancel(int id, SchedulerOwner requester = SCHEDULER_OWNER_CLIENT);

        void handle(); // call once per Core::handle() loop

    private:
        struct Entry {
            int id;
            boolean recurring;
            unsigned long interval;
            unsigned long nextEvent;
            void (*callback)();
            SchedulerOwner owner;
            Entry* next;
        };
        Entry* _head = nullptr;
        int _nextId = 0;
};
