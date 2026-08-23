#include "Scheduler.h"

Scheduler::~Scheduler(){
    while (_head){
        Entry* next = _head->next;
        delete _head;
        _head = next;
    }
}

int Scheduler::schedule(unsigned long intervalMs, void (*callback)(), boolean recurring, SchedulerOwner owner){
    Entry* entry = new Entry();
    entry->id = _nextId++;
    entry->recurring = recurring;
    entry->interval = intervalMs;
    entry->nextEvent = millis() + intervalMs;
    entry->callback = callback;
    entry->owner = owner;
    entry->next = _head;
    _head = entry;
    return entry->id;
}

void Scheduler::cancel(int id, SchedulerOwner requester){
    Entry* prev = nullptr;
    Entry* entry = _head;
    while (entry){
        if (entry->id == id){
            // Client code may not cancel a Core-owned entry.
            if (entry->owner == SCHEDULER_OWNER_CORE && requester != SCHEDULER_OWNER_CORE){
                return;
            }
            if (prev){
                prev->next = entry->next;
            } else {
                _head = entry->next;
            }
            delete entry;
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

void Scheduler::handle(){
    unsigned long now = millis();
    Entry* prev = nullptr;
    Entry* entry = _head;
    while (entry){
        Entry* next = entry->next;
        if (now >= entry->nextEvent){
            boolean oneShot = !entry->recurring;
            if (entry->recurring){
                // Drift-corrected: base the next event on the last
                // scheduled time, not "now", so a late tick doesn't push
                // every later tick back too - matches Mqtt::handle()'s
                // heartbeat scheduling.
                entry->nextEvent += entry->interval;
            }
            entry->callback();
            if (oneShot){
                if (prev){
                    prev->next = next;
                } else {
                    _head = next;
                }
                delete entry;
                entry = nullptr; // don't advance prev onto the freed node
            }
        }
        if (entry){
            prev = entry;
        }
        entry = next;
    }
}
