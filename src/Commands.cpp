#include "Commands.h"
#include "buildConfig.h"

CommandList::CommandList(){
}

String CommandList::toCase(String originalString){
    if (!this->caseSensitive){
        originalString.toLowerCase();
    }
    return originalString;
}

String CommandList::keyFromString(String candidate){
    int index = candidate.indexOf(" ");
    if (index == -1){
        return candidate;
    }
    return candidate.substring(0, index);
}

String CommandList::payloadFromString(String candidate){
    int index = candidate.indexOf(" ");
    if (index == -1){
        return "";
    }
    return candidate.substring(index + 1);
}

boolean CommandList::add(String key, std::function<void(String)> handler){
    Command* item = new Command();
    item->key = this->toCase(key);
    item->handler = handler;
    item->next = this->head;
    this->head = item;
    this->length++;
    return true;
}

void CommandList::list(){
    Command* item = this->head;
    while (item != NULL){
        SER.println(item->key);
        item = item->next;
    }
}

Command* CommandList::getCommand(String candidate){
    String candidateKey = this->keyFromString(this->toCase(candidate));

    Command* item = this->head;
    while (item != NULL){
        if (candidateKey == item->key){
            return item;
        }
        item = item->next;
    }
    return NULL;
}

std::function<void(String)> CommandList::getHandler(String candidate){
    Command* item = getCommand(candidate);
    if (item != NULL){
        return item->handler;
    }
    return std::function<void(String)>();
}

boolean CommandList::runCommand(String candidate){
    Command* item = getCommand(candidate);
    if (item == NULL || !item->handler){
        return false;
    }

    String key = item->key;
    String payload = this->payloadFromString(candidate);

    SER.print("MQTT -> ");
    SER.print(key);
    SER.print(" [");
    SER.print(payload);
    SER.println("]");

    unsigned long timeStart = millis();
    item->handler(payload);
    unsigned long timeTaken = millis() - timeStart;

    SER.print("MQTT -> ");
    SER.print(key);
    SER.print(" executed in ");
    SER.print(timeTaken);
    SER.println(" ms");

    return true;
}
