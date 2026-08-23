/*
    Commands - named MQTT command registry & dispatch

    Lets an application (or Core itself) register a named command with a
    callback, then dispatch an incoming MQTT payload like "<key> <rest>"
    against the registry - the matching callback is called with just the
    "<rest>" portion. Ported from EFXC/effects_controller's own
    Command/CommandList (several of that codebase's modules - lightning,
    skull_eyes, pwm, wlan, etc. - each registered their own commands this
    way), moved into Core since the registry/dispatch mechanism itself is
    foundational, not specific to any one application's effects. The
    profile-filtered add() overload from the original wasn't ported -
    which profile a command applies to is an application-level decision,
    made by the caller choosing whether to call add() at all, not
    something this library needs to know about.

    handler is std::function<String> rather than a raw function pointer
    so both plain free functions (an application's own callbacks) and
    capturing lambdas (e.g. Core's own built-in commands, which need to
    reach Core's own members) work the same way.
*/
#pragma once

#include <Arduino.h>
#include <functional>

class Command{
    public:
        Command* next;                      /* pointer to next command in the list */
        String key;                         /* string (usually single word) that is the command itself */
        std::function<void(String)> handler; /* called with the payload once key is matched */
};

/* Linked list of Command items, matched case-insensitively by default */
class CommandList{
    public:
        CommandList();
        boolean add(String key, std::function<void(String)> handler);
        Command * getCommand(String candidate);   /* search list for a command based on candidate, return pointer to matched Command or NULL */
        std::function<void(String)> getHandler(String candidate); /* search list for a command based on candidate string, return handler or an empty std::function if none matched */
        boolean runCommand(String candidate);      /* split candidate into key + payload, run the matching handler if any - returns false if no match */
        void list();                               /* print every registered key via SER */

    private:
        String toCase(String);
        String keyFromString(String candidate);
        String payloadFromString(String candidate);
        Command* head = NULL;
        int length = 0;
        boolean caseSensitive = false;
};
