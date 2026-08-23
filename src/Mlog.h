// include this for logging (production logging) and debugging (no production)
//

#include "buildConfig.h"
#include "Mqtt.h"

#ifdef INCLUDE_DEBUG
    // RemoteDebug (JoaoLopesF/RemoteDebug) is unmaintained (last commit 2019) -
    // swapped for the actively-maintained lennarthennigs/ESPTelnet
    #include <ESPTelnet.h>
    extern ESPTelnet Telnet;
#endif

class Mlog{
    public:
        enum Level {disabled=0, errors, warning, info, debug, verbose};
        Mlog(void);
        void begin(char *);
        void begin(char *, Mqtt *);
        // log(const char*)/log(String) (used by almost every call site in
        // this codebase) are always emitted, regardless of setLevel() -
        // Mlog's core purpose is a "syslog"-style log (see Mlog.cpp's file
        // header), not a debug-only feature, so unleveled calls stay
        // unconditionally visible rather than silently defaulting to a
        // level that could suppress them. Only the explicit log(Level,
        // String) overload is filtered against the threshold set here.
        void log(const char*);
        void log(String);
        void log(Level, String);
        /*void logf(char *str, ...);*/
        void setLevel(Level);    // change the log(Level, String) filter threshold (default: warning)
        void setMqttClient(Mqtt *);
        void startRemoteDebug();
        void handle();
    private:
        char * deviceId;
        char * generalBuffer;
        Mqtt * mqttClient = 0;
        Level level = warning;
        
};

