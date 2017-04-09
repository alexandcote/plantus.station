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

std::map<std::string, uint64_t> xbeeToPotMap; // map of known pot with their 64 bits addresses
api::Operation* operations = new api::Operation[api::maxOperationsLength];
XBeeZB xBee = XBeeZB(p13, p14, p8, NC, NC, XBEE_BAUD_RATE);
EthernetInterface net;

void ReadConfigFile(uint16_t *panID) {
    INFO_PRINTXNL(INFO, "\r\nReading configuration file...");
    
    char configurationValue[BUFSIZ];
    cfg.read(CFG_PATH);

    cfg.getValue(CFG_KEY_PAN_ID, configurationValue, BUFSIZ);
    *panID = strtol(configurationValue, NULL, HEXA_BASE);
    INFO_PRINTXYNL(INFO, "Pan ID: '0x%X'", *panID);

    cfg.getValue(CFG_KEY_PLACE_IDENTIFIER, configurationValue, BUFSIZ);
    int i = 0;
    while(configurationValue[i] != '\0') {
        placeIdentifier[i] = configurationValue[i];
        i++;
    }

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

void NewFrameReceivedHandler(const RemoteXBeeZB& remoteNode, bool broadcast, const uint8_t *const frame, uint16_t frameLength) {
    INFO_PRINTXNL(INFO, "New frame received!");
    uint64_t remote64Adress = remoteNode.get_addr64();
    uint32_t highAdr = remote64Adress >> 32;
    uint32_t lowAdr = remote64Adress;

    DEBUG_PRINTXYZNL(DEBUG, "\r\nGot a %s RX packet of length '%d'", broadcast ? "BROADCAST" : "UNICAST", frameLength);
    DEBUG_PRINTXYZNL(DEBUG, "16 bit remote address is: '0x%X' and it is '%s'", remoteNode.get_addr16(), remoteNode.is_valid_addr16b() ? "valid" : "invalid");
    DEBUG_PRINTXYZ(DEBUG, "64 bit remote address is:  '0x%X%X'", highAdr, lowAdr);
    DEBUG_PRINTXYNL(DEBUG, "and it is '%s'", remoteNode.is_valid_addr64b() ? "valid" : "invalid");
    DEBUG_PRINTX(DEBUG, "Frame is: ");
    for (int i = 0; i < frameLength; i++)
        DEBUG_PRINTXY(DEBUG, "0x%X ", frame[i]);
    DEBUG_PRINTX(DEBUG, "\r\n");

    switch(frame[0]) {
        case FRAME_PREFIX_ADD_POT_IDENTIFIER:
            INFO_PRINTXNL(INFO, "Add pot identifier to map frame detected!");
            char potIdentifier[PLACE_IDENTIFIER_LENGTH];
            memset(potIdentifier, 0, sizeof(potIdentifier)); // start with fresh values
            for(int i = 1; i < frameLength; i++) 
                    potIdentifier[i-1] = frame[i];
            AddPotIdentifierToMap(potIdentifier, remote64Adress);
            break;
        case FRAME_PREFIX_COMPLETED_OPERATION:
            INFO_PRINTXNL(INFO, "operation completed frame detected!");
            char operationId[OPERATION_ID_MAX_LENGTH];
            memset(operationId, 0, sizeof(operationId)); // start with fresh values
            for(int i = 1; i < frameLength; i++) 
                    operationId[i-1] = frame[i];
            PostCompletedOperation(operationId);
        case FRAME_PREFIX_NEW_DATA:
            INFO_PRINTXNL(INFO, "new time series data frame detected!");
            char data[FRAME_DATA_LENGTH];
            memset(data, 0, sizeof(data)); // start with fresh values
            for(int i = 1; i < frameLength; i++)
                data[i-1] = frame[i];
            PostTimeSeriesData(data, remote64Adress);
            break;
        default:
            INFO_PRINTXYNL(INFO, "Unknown frame '0x%X' detected, nothing will be done!", frame[0]);
            break;
    }
}

std::map<std::string, uint64_t>::iterator serachByValue(std::map<std::string, uint64_t> &xbeeToPotMap, uint64_t val) {
    // Iterate through all elements in std::map and search for the passed element
    std::map<std::string, uint64_t>::iterator it = xbeeToPotMap.begin();
    while(it != xbeeToPotMap.end())
    {
        if(it->second == val)
        return it;
        it++;
    }
}

void AddPotIdentifierToMap(const char potIdentifier[], const uint64_t remote64Adress) {
    if(xbeeToPotMap.find(potIdentifier) != xbeeToPotMap.end()) {
        INFO_PRINTXNL(INFO, "Pot identifier already in map, no need to do anything!");
    } else {
        INFO_PRINTXNL(INFO, "New pot identifier! lets add it to the map!");
        xbeeToPotMap.insert(std::make_pair(potIdentifier, remote64Adress));

        //should now be in map, lets test it
        if(xbeeToPotMap.find(potIdentifier) != xbeeToPotMap.end()) {
            DEBUG_PRINTXNL(DEBUG, "Pot identifier added to map, move on");
        } else {
            DEBUG_PRINTXNL(DEBUG, "Pot identifier was added but is still not in the map!?!?!?!");
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

void PrepareFrameToSend(char frame[], char data[], int framePrefix) {
    memset(frame, 0, sizeof(frame)); // start with fresh values
    frame[0] = framePrefix;
    strcat(frame, data);
}

void CheckIfNewFrameIsPresent(void) {
    INFO_PRINTXNL(INFO, "Check If NewFrame Is Present thread started.");
    while(true) {
        xBee.process_rx_frames();
        Thread::wait(100);
    }
}

void SetupEthernet(void) {
    INFO_PRINTXNL(INFO, "\r\nSetting up Ethernet...");
    net.init();
    net.connect();
    const char *ip = net.getIPAddress();
    INFO_PRINTXY(INFO, "IP address is: %s\r\n", ip ? ip : "No IP");
    INFO_PRINTXNL(INFO, "Ethernet setup finished successfully!\r\n");
}

void GetOperations(void) {
    INFO_PRINTXNL(INFO, "Get operations thread started.");
    while(true) {
        char HTTPresponse[HTTP_RESPONSE_LENGTH];
        api::get("operations/?completed=0", placeIdentifier, operations);
        OperationsParser();
        Thread::wait(10000);
    }
}

void PostCompletedOperation(const char operationId[]) {
    char path[100];
    char emptyBody[1];
    strcpy(path, operationsPath);
    strcat(path, operationId);
    strcat(path, operationCompleted);
    api::post(path, emptyBody, placeIdentifier);
}

int GetPotIdentiferWithRemote64Address(char potIdentifier[], const uint64_t remote64Adress) {
    INFO_PRINTXNL(INFO, "getting pot identifer in the map with remote64Adress...");
    int returnValue = -1;
    std::map<std::string, uint64_t>::iterator it = serachByValue(xbeeToPotMap, remote64Adress);
    if(it != xbeeToPotMap.end()) {
        strcpy(potIdentifier, it->first.c_str());
        returnValue = 1;
        INFO_PRINTXYNL(INFO, "pot Identifier is '%s'", potIdentifier);
    } else {
        uint32_t highAdr = remote64Adress >> 32;
        uint32_t lowAdr = remote64Adress;
        INFO_PRINTXYZNL(INFO, "did not find pot identifier with address '0x%X%X'", highAdr, lowAdr);
    }
    return returnValue;
}

void PostTimeSeriesData(const char data[], const uint64_t remote64Adress) {
    char potIdentifier[POT_IDENTIFIER_LENGTH];

    if(GetPotIdentiferWithRemote64Address(potIdentifier, remote64Adress) > 0) {
        int dataIndex = 0;
        // convert luminosity
        uint16_t luminosityPercent = data[dataIndex];
        INFO_PRINTXYNL(INFO, "posting luminosity percent = '%d'", luminosityPercent);
        dataIndex += LUMINOSITY_DATA_LENGTH;

        // convert temperature
        char temperatureBuffer[TEMPERATURE_DATA_LENGTH];

        for(int i = 0; i < TEMPERATURE_DATA_LENGTH; i++)
            temperatureBuffer[i] = data[i + dataIndex];
            
        float temperature;
        sscanf(temperatureBuffer, "%f", &temperature);
        INFO_PRINTXYNL(INFO, "posting temperature = '%4.2f'", temperature);
        dataIndex += TEMPERATURE_DATA_LENGTH;
        
        // convert soi lHumidity
        uint16_t soilHumidity = data[dataIndex];
        INFO_PRINTXYNL(INFO, "posting soilHumidity = '%d'", soilHumidity);
        dataIndex += SOIL_HUMIDITY_DATA_LENGTH;

        // convert water Level
        uint16_t waterLevel = data[dataIndex];
        INFO_PRINTXYNL(INFO, "posting waterLevel = '%d'", waterLevel);

        // creating body
        char body[200];
        sprintf(body, "{\"temperature\":\"%4.2f\",\"humidity\":\"%i\",\"luminosity\":\"%i\",\"water_level\":\"%i\",\"pot_identifier\":\"%s""\"\}", temperature, soilHumidity, luminosityPercent, waterLevel, potIdentifier);
        DEBUG_PRINTXYNL(INFO, "body is '%s'", body);

        api::post("timeseries/", body, placeIdentifier);
    }
}

void OperationsParser() {
    for(int i = 0; i < api::maxOperationsLength; i++) {
        /*
        // for debug without api only
        strcpy(operations[i].id, "80");
        strcpy(operations[i].potIdentifer, "09bcf78c-5dbb-4348-86f4-17853248c65c");
        char frameAlternatePumpState[FRAME_PREFIX_LENGTH + OPERATION_ID_MAX_LENGTH];
        memset(frameAlternatePumpState, 0, sizeof frameAlternatePumpState); // start with fresh values
        frameAlternatePumpState[0] = FRAME_PREFIX_ALTERNATE_WATER_PUMP_STATE;
        strcat(frameAlternatePumpState, operations[i].id);               
        SendFrameToRemote64BitsAdr(remote64BitsAdr, frameAlternatePumpState, strlen(frameAlternatePumpState));
        */
        // is there a operation to parse?
        if(!strcmp(operations[i].id, "") == 0) {
            DEBUG_PRINTXYZ(DEBUG, "Operation pot identifier '%s'' with id '%s'", operations[i].potIdentifer, operations[i].id);
            DEBUG_PRINTXY(DEBUG, "and action '%s'", operations[i].action);

            // is pot identifier known?
            if(xbeeToPotMap.find(operations[i].potIdentifer) != xbeeToPotMap.end()) {
                uint64_t remote64BitsAdr = xbeeToPotMap.find(operations[i].potIdentifer)->second;

                #if DEBUG
                    uint32_t highAdr = remote64BitsAdr>> 32;
                    uint32_t lowAdr = remote64BitsAdr;
                    DEBUG_PRINTXNL(DEBUG, "Pot identifier is known!");
                    DEBUG_PRINTXYZNL(DEBUG, "remote64BitsAdr is '0x%X%X'", highAdr, lowAdr);
                #endif

                // is action water?
                if( strcmp(operations[i].action, actionWater) == 0 ) {
                    INFO_PRINTXNL(INFO, "Water action detected, sending to defined pot!");
                    char frameWaterPlant[FRAME_PREFIX_LENGTH + OPERATION_ID_MAX_LENGTH];
                    PrepareFrameToSend(frameWaterPlant, operations[i].id, FRAME_PREFIX_TURN_WATER_PUMP_ON);      
                    SendFrameToRemote64BitsAdr(remote64BitsAdr, frameWaterPlant, strlen(frameWaterPlant));
                } else {
                    INFO_PRINTXYNL(INFO, "Unknown action '%s' detected, nothing will be done!", operations[i].action);
                }

             } else {
                 INFO_PRINTXYNL(INFO, "'%s' was not found in the map, operation will not be sent...", operations[i].potIdentifer);
             }
        }
    }
}

int main() {
    INFO_PRINTXNL(DEBUG, "Station node started");
    SetLedTo(0, true); // Init LED on
    xbeeToPotMap.insert(std::make_pair("09bcf78c-5dbb-4348-86f4-17853248c65c", 0x0013A20040331988)); // for debug
    ReadConfigFile(&panID);
    SetupEthernet();
    SetupXBee(panID);

    Thread zigBeeFrameReceiveThread(CheckIfNewFrameIsPresent, osPriorityHigh, 3072);
    Thread getOperationsThread(GetOperations, osPriorityNormal, 3072);
    Thread flashLed3Thread;
    flashLed3Thread.start(callback(FlashLed, (void *)3));

    SetLedTo(0, false); // Init LED off
    while (true) {
        Thread::wait(osWaitForever);
    }
}

void MapTests(void) {
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

void SetLedTo(uint16_t led, bool state) {
    LEDs[led] = state;
}

void FlashLed(void const *led) {
    while(true) {
        LEDs[(int)led] = !LEDs[(int)led];
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