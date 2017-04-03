#include "mbed.h"
#include "LPC17xx.h"
#include "rtos.h"
#include "ConfigFile.h"
#include "XBeeLib.h"
#include "EthernetInterface.h"
#include "api.h"
#include "Json.h"
#include <map>
#include <string>

using namespace XBeeLib;

#define DEBUG false
#define INFO true
#define MAC_ADR_LENGTH 6

// used for HTTP
#define HTTP_RESPONSE_LENGTH 1024
#define PLACE_IDENTIFIER_LENGTH 37

// used for XBee
#define XBEE_BAUD_RATE 115200
#define FRAME_PREFIX_TURN_WATER_PUMP_ON 0xBB
#define FRAME_PREFIX_TURN_WATER_PUMP_OFF 0xAA
#define FRAME_PREFIX_ALTERNATE_WATER_PUMP_STATE 0xFF // for debug purposes
#define FRAME_PREFIX_NEW_DATA 0x00
#define FRAME_PREFIX_ADD_POT_IDENTIFIER 0x01
#define FRAME_PREFIX_COMPLETED_OPERATION 0x10
#define FRAME_PREFIX_LENGTH 1

// theses defines must be the same as in the pot project...
#define LUMINOSITY_DATA_LENGTH 2
#define AMBIANT_TEMPERATURE_DATA_LENGTH 1
#define SOIL_HUMIDITY_DATA_LENGTH 1
#define WATER_LEVEL_DATA_LENGTH 1
#define FRAME_DATA_LENGTH FRAME_PREFIX_LENGTH + LUMINOSITY_DATA_LENGTH + AMBIANT_TEMPERATURE_DATA_LENGTH + SOIL_HUMIDITY_DATA_LENGTH + WATER_LEVEL_DATA_LENGTH 

// used for configuration file
#define HEXA_BASE 16
#define CFG_PATH "/local/config.cfg"
#define CFG_KEY_PAN_ID "PAN_ID"
#define CFG_KEY_PLACE_IDENTIFIER "PLACE_IDENTIFIER"

// peripherals
extern Serial pc;
extern DigitalOut LEDs[4];
extern LocalFileSystem local;
extern ConfigFile cfg;
extern EthernetInterface net;

// global variables
char placeIdentifier[PLACE_IDENTIFIER_LENGTH];
char macAdr[MAC_ADR_LENGTH];
const char* operationsPath = "operations/";
const char* operationCompleted = "/completed/";
uint16_t panID; 

// prototypes
void ReadConfigFile(uint16_t *panID);
void SetLedTo(uint16_t led, bool state);
void FlashLed(void const *led);
void SetupXBee(XBeeZB *xBee, uint16_t panID);
void CheckIfNewFrameIsPresent(void);
void NewFrameReceivedHandler(const RemoteXBeeZB& remoteNode, bool broadcast, const uint8_t *const data, uint16_t len);
void SendFrameToRemote64BitsAdr(uint64_t remote64BitsAdr, char frame[], uint16_t frameLength);
void OperationsParser();
void AddPotIdentifierToMap(const char potIdentifier[], const uint64_t remote64Adress);
void PostCompletedOperation(const char operationId[]);
void PostTimeSeriesData(const char data[], const uint64_t remote64Adress);
std::map<std::string, uint64_t>::iterator serachByValue(std::map<std::string, uint64_t> &xbeeToPotMap, uint64_t val);