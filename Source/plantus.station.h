#pragma once 

#include "mbed.h"
#include "LPC17xx.h"
#include "rtos.h"
#include "ConfigFile.h"
#include "XBeeLib.h"
#include "EthernetInterface.h"
#include "plantus.api.h"
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
#define ACTION_WATER "water"
#define STATIC_ADR false

// used for XBee
#define XBEE_BAUD_RATE 115200
#define FRAME_PREFIX_WATER_PLANT 0xBB
#define FRAME_PREFIX_TURN_WATER_PUMP_OFF 0xAA
#define FRAME_PREFIX_ALTERNATE_WATER_PUMP_STATE 0xFF // for debug purposes
#define FRAME_PREFIX_NEW_DATA 0x00
#define FRAME_PREFIX_ADD_POT_IDENTIFIER 0x01
#define FRAME_PREFIX_COMPLETED_OPERATION 0x10

// must be the same in pot...
#define LUMINOSITY_DATA_LENGTH 1
#define TEMPERATURE_DATA_LENGTH 6
#define SOIL_HUMIDITY_DATA_LENGTH 1
#define WATER_LEVEL_DATA_LENGTH 1
#define FRAME_PREFIX_LENGTH 1
#define FRAME_DATA_LENGTH FRAME_PREFIX_LENGTH + LUMINOSITY_DATA_LENGTH + TEMPERATURE_DATA_LENGTH + SOIL_HUMIDITY_DATA_LENGTH + WATER_LEVEL_DATA_LENGTH 

// used for configuration file
#define HEXA_BASE 16
#define CFG_PATH "/local/config.cfg"
#define CFG_KEY_PAN_ID "PAN_ID"
#define CFG_KEY_PLACE_IDENTIFIER "PLACE_IDENTIFIER"
#define ZIGBEE_STACK_SIZE 512*5
#define OPERATIONS_STACK_SIZE 512*5

// global variables

// peripherals
Serial pc(USBTX, USBRX);   // tx, rx
DigitalOut LEDs[4] = {
    DigitalOut(LED1), DigitalOut(LED2), DigitalOut(LED3), DigitalOut(LED4)
};
LocalFileSystem local("local");
ConfigFile cfg;
EthernetInterface net;
XBeeZB xBee = XBeeZB(p13, p14, p8, NC, NC, XBEE_BAUD_RATE);

// values
Thread *ptrZigBeeThread;
Thread *ptrGetOperationsThread;
std::map<std::string, uint64_t> xbeeToPotMap; // map of known pot with their 64 bits addresses
api::Operation* operations = new api::Operation[api::maxOperationsLength];
char placeIdentifier[PLACE_IDENTIFIER_LENGTH];
const char* operationsPath = "operations/";
const char* operationCompleted = "/completed/";
const char* actionWater = "water";
const char* staticIP = "192.168.000.002";
const char* staticMask = "255.255.255.000";
const char* staticGateway = "192.168.000.001";

// prototypes
void ReadConfigFile(uint16_t *panID);
void SetupXBee(XBeeZB *xBee, uint16_t panID);
void NewFrameReceivedHandler(const RemoteXBeeZB& remoteNode, bool broadcast, const uint8_t *const data, uint16_t len);
std::map<std::string, uint64_t>::iterator serachByValue(std::map<std::string, uint64_t> &xbeeToPotMap, uint64_t val);
void AddPotIdentifierToMap(const char potIdentifier[], const uint64_t remote64Adress);
void SendFrameToRemote64BitsAdr(const uint64_t remote64BitsAdr, const char frame[], const uint16_t frameLength);
void PrepareFrameToSend(char frame[], const char data[], const int framePrefix);
void CheckIfNewFrameIsPresent(void);
void SetupEthernet(void);
void GetOperations(void);
void PostCompletedOperation(const char operationId[]);
int GetPotIdentiferWithRemote64Address(char potIdentifier[], const uint64_t remote64Adress);
void PostTimeSeriesData(const char data[], const uint64_t remote64Adress);
float GetFloatFromData(const char data[], int startDataIndex, int lengthOfFloat);
void OperationsParser(void);
void SetLedTo(uint16_t led, bool state);
void FlashLed(void const *led);
void GetMacAddress(char *macAdr);

// for debug
#define DEBUG_PRINTX(DEBUG, x) if(DEBUG) {pc.printf(x);}
#define DEBUG_PRINTXNL(DEBUG, x) if(DEBUG) {pc.printf(x);               pc.printf("\r\n");}
#define DEBUG_PRINTXY(DEBUG, x, y) if(DEBUG) {pc.printf(x, y);}
#define DEBUG_PRINTXYNL(DEBUG, x, y) if(DEBUG) {pc.printf(x, y);         pc.printf("\r\n");}
#define DEBUG_PRINTXYZ(DEBUG, x, y, z) if(DEBUG) {pc.printf(x, y, z);}
#define DEBUG_PRINTXYZNL(DEBUG, x, y, z) if(DEBUG) {pc.printf(x, y, z);  pc.printf("\r\n");}
#define INFO_PRINTX(INFO, x) if(INFO) {pc.printf(x);}
#define INFO_PRINTXNL(INFO, x) if(INFO) {pc.printf(x);               pc.printf("\r\n");}
#define INFO_PRINTXY(INFO, x, y) if(INFO) {pc.printf(x, y);}
#define INFO_PRINTXYNL(INFO, x, y) if(INFO) {pc.printf(x, y);         pc.printf("\r\n");}
#define INFO_PRINTXYZ(INFO, x, y, z) if(INFO) {pc.printf(x, y, z);}
#define INFO_PRINTXYZNL(INFO, x, y, z) if(INFO) {pc.printf(x, y, z);  pc.printf("\r\n");}