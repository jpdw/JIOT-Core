// Enable/disable build-time compilation options

// Include ability to use ESPTOOL to push new software OTA to the device
#define INCLUDE_OTA_PUSH

// Whether to include DEBUG features in the code
//#define INCLUDE_DEBUG
// Which serial port to use for debug ouput (usually Serial1)
#define SER Serial

#define SERIAL_BAUD 115200  /* default serial debug baud rate */
#define APP_STRING "uNode2" /* prepend hostname with this */

// Frequency of schedule overflows (old scheduler in main.cpp)
#define TEMPERATURE_INTERVAL 60000
#define HELLO_INTERVAL 120000
#define DEFAULT_WEBSUBMIT_INTERVAL 5000
#define DEFAULT_LEDFLASH_INTERVAL 2000
#define DEFAULT_RETRVTEMP_INTERVAL 750 /* should be 750 */
#define DEFAULT_SENDHBEAT_INTERVAL 10000 /* 10 seconds */

#define DEFAULT_10SECOND_INTERVAL 10000
#define DEFAULT_3SECOND_INTERVAL 3000
#define DEFAULT_2SECOND_INTERVAL 2000
#define DEFAULT_1SECOND_INTERVAL 1000

// Old command object (main.cpp)
#define CONSOLE_CMD_STRMAX 10
#define FIXED_COMMAND_SLOTS 10

// DS18B20 data retrieval & publishing
#define PUBLISH_TEMPERATURE true

// Device/Node IDs
#define NODE_NAME_BASE "uNode2"
#define NODE_NAME_TEMPLATE "%s-%06X"
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


