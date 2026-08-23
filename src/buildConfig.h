// Enable/disable build-time compilation options

// Include ability to use ESPTOOL to push new software OTA to the device
#define INCLUDE_OTA_PUSH

// Whether to include DEBUG features in the code
//#define INCLUDE_DEBUG
// Which serial port to use for debug ouput (usually Serial1)
#define SER Serial

#define TOPIC_CONTEXT_INITIAL "device"  /* top-level topic for initial broacast */
#define TOPIC_CONTEXT "home"            /* top-level topic to use generally */

/* On start-up:
    pub to: device/<deviceid>/hello      payload = core version, topic context, ip address, rssi, essid
    sub to: device/<deviceid>/cmnd       ]
            device/$ALL/cmnd             ]- process both equally (?)

   Then, after that, all pubs are to
    <context>/<deviceid>/<subtopic>
    - where <context> is one of
       - "home" for home-tech (HA)
       - "allot" for allotment

*/


