#include "plantus.station.h"

// For debug
Serial pc(USBTX, USBRX);   // tx, rx
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

DigitalOut LEDs[4] = {
    DigitalOut(LED1), DigitalOut(LED2), DigitalOut(LED3), DigitalOut(LED4)
};
ConfigFile cfg;
LocalFileSystem local("local");

std::map<std::string, uint64_t> xbeeToPotMap;
api::Operation* operations = new api::Operation[api::maxOperationsLength];
XBeeZB xBee = XBeeZB(p13, p14, p8, NC, NC, XBEE_BAUD_RATE);
EthernetInterface net;

void SetLedTo(uint16_t led, bool state) {
    LEDs[led] = state;
}

void FlashLed(void const *led) {
    LEDs[(int)led] = !LEDs[(int)led];
}

void FlashLed3() {
    while(true){
        LEDs[3] = !LEDs[3];
        Thread::wait(1000);
    }
}

void GetMacAddress(char *macAdr) {
    mbed_mac_address(macAdr);
    #if DEBUG
    DEBUG_PRINTX(DEBUG, "\r\nmBed Mac Adress = ");
     for(int i=0; i<MAC_ADR_LENGTH; i++) {
        DEBUG_PRINTXY(DEBUG, "%02X ", macAdr[i]);      
    }   
    DEBUG_PRINTXNL(DEBUG, "\n\r");
    #endif
}

void ReadConfigFile(uint16_t *panID) {
    INFO_PRINTXNL(INFO, "\r\nReading configuration file...");
    int i = 0;
    char configurationValue[BUFSIZ];
    cfg.read(CFG_PATH);

    cfg.getValue(CFG_KEY_PAN_ID, configurationValue, BUFSIZ);
    *panID = strtol(configurationValue, NULL, HEXA_BASE);
    INFO_PRINTXYNL(INFO, "Pan ID: '0x%X'", *panID);

    cfg.getValue(CFG_KEY_PLACE_IDENTIFIER, configurationValue, BUFSIZ);
    
    while(configurationValue[i] != '\0') {
        placeIdentifier[i] = configurationValue[i];
        i++;
    }
    i = 0;
    INFO_PRINTXYNL(INFO, "Place identifier: '%s'", placeIdentifier);
    INFO_PRINTXNL(INFO, "Reading configuration file finished successfully!!\r\n");
}

void SetupXBee(uint16_t panID) {
    INFO_PRINTXNL(INFO, "\r\nInitiating XBee...");

    xBee.register_receive_cb(&NewFrameReceivedHandler);
    
    RadioStatus radioStatus = xBee.init();
    MBED_ASSERT(radioStatus == Success);

    uint64_t Adr64Bits = xBee.get_addr64();
    uint32_t highAdr = Adr64Bits >> 32;
    uint32_t lowAdr = Adr64Bits;
    INFO_PRINTXYZNL(INFO, "Primary initialization successful, device 64bit Adress = '0x%X%X'", highAdr, lowAdr);

    radioStatus = xBee.set_panid(panID);
    MBED_ASSERT(radioStatus == Success);
    INFO_PRINTXYNL(INFO, "Device PanID was set to '0x%X'", panID);

    INFO_PRINTXY(INFO, "Waiting for device to create the network '0x%X'", panID);
    while (!xBee.is_joined()) {
        wait_ms(1000);
        INFO_PRINTX(INFO, ".");
    }
    INFO_PRINTXYNL(INFO, "\r\ndevice created network with panID '0x%X' successfully!", panID);

    INFO_PRINTXNL(INFO, "XBee initialization finished successfully!\n\r");
}

void NewFrameReceivedHandler(const RemoteXBeeZB& remoteNode, bool broadcast, const uint8_t *const frame, uint16_t frameLength)
{
    INFO_PRINTXNL(INFO, "New frame received!");
    uint64_t remote64Adress = remoteNode.get_addr64();
    uint32_t highAdr = remote64Adress >> 32;
    uint32_t lowAdr = remote64Adress;

    DEBUG_PRINTXYZNL(DEBUG, "\r\nGot a %s RX packet of length '%d'", broadcast ? "BROADCAST" : "UNICAST", frameLength);
    DEBUG_PRINTXYZNL(DEBUG, "16 bit remote address is: '0x%X' and it is '%s'", remoteNode.get_addr16(), remoteNode.is_valid_addr16b() ? "valid" : "invalid");
    DEBUG_PRINTXYZ(DEBUG, "64 bit remote address is:  '0x%X%X'", highAdr, lowAdr);
    DEBUG_PRINTXYNL(DEBUG, "and it is '%s'", remoteNode.is_valid_addr64b() ? "valid" : "invalid");
    DEBUG_PRINTX(INFO, "Frame is: ");
    for (int i = 0; i < frameLength; i++)
        DEBUG_PRINTXY(INFO, "0x%X ", frame[i]);
    DEBUG_PRINTX(INFO, "\r\n");

    switch(frame[0]) {
        case FRAME_PREFIX_ADD_POT_IDENTIFIER:
            INFO_PRINTXNL(INFO, "Add pot identifier to map frame detected!");
            char potIdentifier[PLACE_IDENTIFIER_LENGTH];
            memset(potIdentifier, 0, sizeof potIdentifier); // start with fresh values
            for(int i = 1; i < frameLength; i++) 
                    potIdentifier[i-1] = frame[i];
            AddPotIdentifierToMap(potIdentifier, remote64Adress);
            break;
        case FRAME_PREFIX_COMPLETED_OPERATION:
            INFO_PRINTXNL(INFO, "operation completed frame detected!");
            char operationId[OPERATION_ID_MAX_LENGTH];
            memset(operationId, 0, sizeof operationId); // start with fresh values
            for(int i = 1; i < frameLength; i++) 
                    operationId[i-1] = frame[i];
            PostCompletedOperation(operationId);
        case FRAME_PREFIX_NEW_DATA:
            INFO_PRINTXNL(INFO, "new time series data frame detected!");
            char data[FRAME_DATA_LENGTH];
            memset(data, 0, sizeof data); // start with fresh values
            for(int i = 1; i < frameLength; i++)
                data[i-1] = frame[i];
            PostTimeSeriesData(data, remote64Adress);
            break;
        default:
            INFO_PRINTXYNL(INFO, "Unknown frame '0x%X' detected, nothing will be done!", frame[0]);
            break;
    }
    // TODO process other received data here!
}

void AddPotIdentifierToMap(const char potIdentifier[], const uint64_t remote64Adress) {
    if(xbeeToPotMap.find(potIdentifier) != xbeeToPotMap.end()) {
        INFO_PRINTXNL(INFO, "Pot identifier already in map, no need to do anything!");
    } else {
        INFO_PRINTXNL(INFO, "New pot identifier! lets add it to the map!");
        xbeeToPotMap.insert(std::make_pair(potIdentifier, remote64Adress));

        //should now be in map, lets test it
        if(xbeeToPotMap.find(potIdentifier) != xbeeToPotMap.end()) {
            INFO_PRINTXNL(INFO, "Pot identifier added to map, move on");
        } else {
            INFO_PRINTXNL(INFO, "Pot identifier was added but is still not in the map!?!?!?!");
        }
    }
}

void SendFrameToRemote64BitsAdr(uint64_t remote64BitsAdr, char frame[], uint16_t frameLength) {
    DEBUG_PRINTXNL(DEBUG, "sending frame :");
    for(int i = 0; i < frameLength; i++) {
        DEBUG_PRINTXYZNL(DEBUG, "data at index '%i' = '0x%X'", i, frame[i]);
    }
    
    RemoteXBeeZB remoteNode = RemoteXBeeZB(remote64BitsAdr);
    if(remoteNode.is_valid()) {
        TxStatus txStatus = xBee.send_data(remoteNode, (const uint8_t *)frame, frameLength);
        if (txStatus == TxStatusSuccess) {
            INFO_PRINTXNL(INFO, "Frame sent to remote node successfully!\r\n");
        } else {
            INFO_PRINTXYNL(INFO, "Failed to send frame to remote node with error '%d', look in XBee.h for more details\r\n", (int) txStatus);
        }
    } else {
        INFO_PRINTXYNL(INFO, "Did not find device with address '0x%X'\r\n", remote64BitsAdr);
    }
}

void CheckIfNewFrameIsPresent(void) {
    INFO_PRINTXNL(INFO, "Check If NewFrame Is Present thread started.");
    while(true) {
        xBee.process_rx_frames();
        Thread::wait(100);
    }
}

void SetupEthernet() {
    INFO_PRINTXNL(INFO, "\r\nSetting up Ethernet...");
    net.init();
    net.connect();
    const char *ip = net.getIPAddress();
    INFO_PRINTXY(INFO, "IP address is: %s\r\n", ip ? ip : "No IP");
    INFO_PRINTXNL(INFO, "Ethernet setup finished successfully!\r\n");
}

void GetOperations() {
    INFO_PRINTXNL(INFO, "Get operations thread started.");
    while(true) {
        char HTTPresponse[HTTP_RESPONSE_LENGTH];
        api::get("operations/?completed=0", placeIdentifier, operations);
        OperationsParser();
        Thread::wait(5000);
    }
}

void PostCompletedOperation(const char operationId[]) {
    char path[100];
    char emptyBody[1];
    strcpy(path, operationsPath);
    strcat(path, operationId);
    strcat(path, operationCompleted);
    DEBUG_PRINTXYNL(DEBUG, "Post operations path is '%s'.", path);
    api::post(path, emptyBody, placeIdentifier);
}

// TODO WIP -> convert the data and create the body
void PostTimeSeriesData(const char data[], const uint64_t remote64Adress) {
    INFO_PRINTXNL(INFO, "getting pot identifer in the map with remote64Adress...");
    std::map<std::string, uint64_t>::iterator it = serachByValue(xbeeToPotMap, remote64Adress);
    char potIdentifier[POT_IDENTIFIER_LENGTH];

    DEBUG_PRINTX(INFO, "Data is: ");
    for (int i = 0; i < FRAME_DATA_LENGTH; i++)
        DEBUG_PRINTXY(INFO, "0x%X ", data[i]);
    DEBUG_PRINTX(INFO, "\r\n");

    if(it != xbeeToPotMap.end()) {
            strcpy(potIdentifier, it->first.c_str());
            INFO_PRINTXYNL(INFO, "pot Identifier is '%s'", potIdentifier);

            char* test = "{\"temperature\":\"10.00\",\"humidity\":\"10.00\",\"luminosity\":\"10.00\",\"water_level\":\"10.00\",\"pot_identifier\":\"7ee45d53-df2a-40e8-a2b3-c32ca07348c0\"}";
            INFO_PRINTXYNL(INFO, "test1 is '%s'", test);
            //int test = atoi(data[0]);

            //uint16_t luminosity = atoi(data[0]) << 8; // high
            //luminosity += atoi(data[1]); // low
            //INFO_PRINTXYNL(INFO, "Int luminosity is '0x%08x'", luminosity);

            //int luminosity[LUMINOSITY_DATA_LENGTH] = {data[0], data[1]};
            int ambiantTemperature[AMBIANT_TEMPERATURE_DATA_LENGTH] = {data[2]};
            int soilHumidity[SOIL_HUMIDITY_DATA_LENGTH] = {data[3]};
            int waterLevel[WATER_LEVEL_DATA_LENGTH] = {data[4]};
            
            /*
            char body[200];
            char* temperatureBody = "{\"temperature\":\"";
            strcat(body, temperatureBody);

            char* humidityBody = "\",\"humidity\":\"";
            strcat(body, humidityBody);
            strcat(body, soilHumidity);
            char* luminosityBody = "\",\"luminosity\":\"";
            strcat(body, luminosityBody);
            strcat(body, luminosity);
            char* waterLevelBody = "\",\"water_level\":\"";
            strcat(body, waterLevelBody);
            strcat(body, waterLevel);
            char* potIdentifierBody = "\",\"pot_identifier\":\"";
            strcat(body, potIdentifierBody);
            strcat(body, potIdentifier);
            char* endOfBody = "\"}";
            strcat(body, potIdentifierBody);

            INFO_PRINTXYNL(INFO, "body is '%s'", body);
            */
            DEBUG_PRINTXYNL(INFO, "Posting luminosty high '0x%X'.", luminosity[0]);
            DEBUG_PRINTXYNL(INFO, "Posting luminosty low '0x%X'.", luminosity[1]);
            DEBUG_PRINTXYNL(INFO, "Posting temperature '0x%X'.", ambiantTemperature[0]);
            DEBUG_PRINTXYNL(INFO, "Posting humidity '0x%X'.", soilHumidity[0]);
            DEBUG_PRINTXYNL(INFO, "Posting water_level '0x%X'.", waterLevel[0]);

            /*
            HTTPMap* map;
            map->put("luminosity", luminosity);
            map->put("temperature", ambiantTemperature);
            map->put("humidity", soilHumidity);
            map->put("water_level", waterLevel);
            map->put("pot_identifier", potIdentifier);
            */
            //api::post("timeseries/", body, placeIdentifier);
    } else {
        uint32_t highAdr = remote64Adress >> 32;
        uint32_t lowAdr = remote64Adress;
        INFO_PRINTXYZNL(INFO, "did not find pot identifier with address '0x%X%X'", highAdr, lowAdr);
    }
}

void OperationsParser() {
    for(int i = 0; i < api::maxOperationsLength; i++) {
        /*
        // for debug without api only
        strcpy(operations[i].id, "80");
        strcpy(operations[i].potIdentifer, "09bcf78c-5dbb-4348-86f4-17853248c65c");
        */
        if(!strcmp(operations[i].id, "") == 0) {
            DEBUG_PRINTXYZNL(DEBUG, "Operation pot identifier '%s'' with id '%s'", operations[i].potIdentifer, operations[i].id);
            
            // is pot known?
            if(xbeeToPotMap.find(operations[i].potIdentifer) != xbeeToPotMap.end()) {
                uint64_t remote64BitsAdr = xbeeToPotMap.find(operations[i].potIdentifer)->second;
                uint32_t highAdr = remote64BitsAdr>> 32;
                uint32_t lowAdr = remote64BitsAdr;
                DEBUG_PRINTXNL(DEBUG, "Pot identifier is known!");
                DEBUG_PRINTXYZNL(DEBUG, "remote64BitsAdr is '0x%X%X'", highAdr, lowAdr);
                DEBUG_PRINTXNL(DEBUG, "Sending operation to pot");

                char frameAlternatePumpState[FRAME_PREFIX_LENGTH + OPERATION_ID_MAX_LENGTH];
                memset(frameAlternatePumpState, 0, sizeof frameAlternatePumpState); // start with fresh values
                frameAlternatePumpState[0] = FRAME_PREFIX_ALTERNATE_WATER_PUMP_STATE;
                strcat(frameAlternatePumpState, operations[i].id);               
                SendFrameToRemote64BitsAdr(remote64BitsAdr, frameAlternatePumpState, strlen(frameAlternatePumpState));
             } else {
                 INFO_PRINTXYNL(INFO, "'%s' was not found in the map, operation will not be sent...", operations[i].potIdentifer);
             }
        }
    }
}

std::map<std::string, uint64_t>::iterator serachByValue(std::map<std::string, uint64_t> &xbeeToPotMap, uint64_t val) 
    // Iterate through all elements in std::map and search for the passed element
    std::map<std::string, uint64_t>::iterator it = xbeeToPotMap.begin();
    while(it != xbeeToPotMap.end())
    {
        if(it->second == val)
        return it;
        it++;
    }
}

void MapTests(void){
    string testPotID = "09bcf78c-5dbb-4348-86f4-17853248c65c";
    uint64_t test64BitsADR = 0x0013A20040331988;
    xbeeToPotMap.insert(std::make_pair(testPotID, test64BitsADR));
    if(xbeeToPotMap.find(testPotID) != xbeeToPotMap.end()){
        DEBUG_PRINTXNL(DEBUG, "found it!");
    } else {
        DEBUG_PRINTXYNL(DEBUG, "did not find '%s'", testPotID);
    }
    
    std::map<std::string, uint64_t>::iterator it = serachByValue(xbeeToPotMap, test64BitsADR);
    if(it != xbeeToPotMap.end()) {
            uint32_t highAdr = it->second >> 32;
            uint32_t lowAdr = it->second;
            char potId[POT_IDENTIFIER_LENGTH];
            strcpy(potId, it->first.c_str());
            DEBUG_PRINTXYZNL(DEBUG, "64 bit adr is 0x%X%X'", highAdr, lowAdr);
            DEBUG_PRINTXYNL(DEBUG, "pot ID is '%s'", potId);
    } else {
        DEBUG_PRINTXYZNL(DEBUG, "did not find value '0x%X%X'", test64BitsADR>>32, test64BitsADR);
    }
}

int main() {
    INFO_PRINTXNL(DEBUG, "Station node started");
    SetLedTo(0, true); // Init LED on

    ReadConfigFile(&panID);
    SetupEthernet();
    SetupXBee(panID);

    Thread zigBeeFrameReceiveThread(CheckIfNewFrameIsPresent, osPriorityHigh, 3072);
    Thread getOperationsThread(GetOperations, osPriorityNormal, 3072);
    Thread flashLed3Thread;
    flashLed3Thread.start(FlashLed3);

    SetLedTo(0, false); // Init LED off
    while (true) {
        Thread::wait(osWaitForever);
    }
}

