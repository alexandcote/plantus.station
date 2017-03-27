#include "mbed.h"
#include "LPC17xx.h"
#include "ConfigFile.h"
#include "XBeeLib.h"
#include "EthernetInterface.h"
#include "api.h"

using namespace XBeeLib;

#define DEBUG true
#define MAC_ADR_LENGTH 6

// used for XBee
#define XBEE_BAUD_RATE 115200
#define NODE_IDENTIFIER_MAX_LENGTH 21
#define PLACE_IDENTIFIER_LENGTH 36
#define MAX_NODES 5
#define NODE_DISCOVERY_TIMEOUT_MS   5000
#define TURN_WATER_PUMP_ON 0xBB
#define TURN_WATER_PUMP_OFF 0xAA
#define ALTERNATE_WATER_PUMP_STATE 0xFF // for debug purposes
#define POT_1 "Patate"
#define POT_1_64_BITS_ADR_HIGH 0x0013A200
#define POT_1_64_BITS_ADR_LOW  0x40331988

// used for configuration file
#define HEXA_BASE 16
#define CFG_PATH "/local/config.cfg"
#define CFG_KEY_PAN_ID "PAN_ID"
#define CFG_KEY_NODE_IDENTIFIER "NODE_IDENTIFIER"
#define CFG_KEY_PLACE_IDENTIFIER "PLACE_IDENTIFIER"

// peripherals
extern Serial pc;
extern DigitalOut LEDs[4];
extern LocalFileSystem local;
extern ConfigFile cfg;
extern EthernetInterface net;

// global variables
RemoteXBeeZB remoteNodesInNetwork[MAX_NODES];
char nodeIdentifier[NODE_IDENTIFIER_MAX_LENGTH];
char placeIdentifier[PLACE_IDENTIFIER_LENGTH];
char macAdr[MAC_ADR_LENGTH];
uint16_t panID; 
uint8_t remoteNodesCount = 0;

// prototypes
void ReadConfigFile(uint16_t *panID);
void GetMacAddress(char *macAdr);
void SetLedTo(uint16_t led, bool state);
void FlashLed(uint16_t led);
void StartEventQueueThread(void);
void SetupXBee(XBeeZB *xBee, uint16_t panID);
void CheckIfNewFrameIsPresent(void);
void NewFrameReceivedHandler(const RemoteXBeeZB& remoteNode, bool broadcast, const uint8_t *const data, uint16_t len);
void discovery_function(const RemoteXBeeZB& remoteNode, char const * const node_id);
void DiscoverNodeById(char *nodeId);
void DiscoverAllNodes(void);
void ChangeRemoteWaterPumpStateById(char *nodeId, bool state);
void ChangeRemoteWaterPumpBy64BitAdr(uint64_t remote64BitsAdr, bool state);
