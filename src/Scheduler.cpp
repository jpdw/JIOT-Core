#include "Scheduler.h"

int Scheduler::schedule(unsigned long intervalMs, void (*callback)(), boolean recurring){
    for (int i = 0; i < SCHEDULER_MAX_SLOTS; i++){
        if (!_slots[i].active){
            _slots[i].active = true;
            _slots[i].recurring = recurring;
            _slots[i].interval = intervalMs;
            _slots[i].nextEvent = millis() + intervalMs;
            _slots[i].callback = callback;
            return i;
        }
    }
    return -1; // every slot in use
}

void Scheduler::cancel(int slot){
    if (slot < 0 || slot >= SCHEDULER_MAX_SLOTS){
        return;
    }
    _slots[slot].active = false;
}

void Scheduler::handle(){
    unsigned long now = millis();
    for (int i = 0; i < SCHEDULER_MAX_SLOTS; i++){
        if (_slots[i].active && now >= _slots[i].nextEvent){
            if (_slots[i].recurring){
                // Drift-corrected: base the next event on the last
                // scheduled time, not "now", so a late tick doesn't push
                // every later tick back too - matches Mqtt::handle()'s
                // heartbeat scheduling.
                _slots[i].nextEvent += _slots[i].interval;
            } else {
                _slots[i].active = false;
            }
            _slots[i].callback();
        }
    }
}
