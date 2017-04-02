#include "plantus.station.h"

// For debug
Serial pc(USBTX, USBRX);   // tx, rx
#define DEBUG_PRINTX(DEBUG, x) if(DEBUG) {pc.printf(x);}
#define DEBUG_PRINTXNL(DEBUG, x) if(DEBUG) {pc.printf(x);               pc.printf("\r\n");}
#define DEBUG_PRINTXY(DEBUG, x, y) if(DEBUG) {pc.printf(x, y);}
#define DEBUG_PRINTXYNL(DEBUG, x, y) if(DEBUG) {pc.printf(x, y);         pc.printf("\r\n");}
#define DEBUG_PRINTXYZ(DEBUG, x, y, z) if(DEBUG) {pc.printf(x, y, z);}
#define DEBUG_PRINTXYZNL(DEBUG, x, y, z) if(DEBUG) {pc.printf(x, y, z);  pc.printf("\r\n");}

// Peripherals
DigitalOut LEDs[4] = {
    DigitalOut(LED1), DigitalOut(LED2), DigitalOut(LED3), DigitalOut(LED4)
};
ConfigFile cfg;
LocalFileSystem local("local");

//Thread zigBeeThread;
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
        Thread::wait(2000);
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
    DEBUG_PRINTXNL(DEBUG, "\r\nReading configuration file...");
    int i = 0;
    char configurationValue[BUFSIZ];
    cfg.read(CFG_PATH);

    cfg.getValue(CFG_KEY_PAN_ID, configurationValue, BUFSIZ);
    *panID = strtol(configurationValue, NULL, HEXA_BASE);
    DEBUG_PRINTXYNL(DEBUG, "Pan ID: '0x%X'", *panID);

    cfg.getValue(CFG_KEY_PLACE_IDENTIFIER, configurationValue, BUFSIZ);
    
    while(configurationValue[i] != '\0') {
        placeIdentifier[i] = configurationValue[i];
        i++;
    }
    i = 0;
    DEBUG_PRINTXYNL(DEBUG, "Place identifier: '%s'", placeIdentifier);

    cfg.getValue(CFG_KEY_NODE_IDENTIFIER, configurationValue, BUFSIZ);

    while(configurationValue[i] != '\0') {
        nodeIdentifier[i] = configurationValue[i];
        i++;
    }
    DEBUG_PRINTXYNL(DEBUG, "node identifier: '%s'", nodeIdentifier);

    DEBUG_PRINTXNL(DEBUG, "Reading configuration file finished successfully!!\r\n");
}

void SetupXBee(uint16_t panID) {
    DEBUG_PRINTXNL(DEBUG, "\r\nInitiating XBee...");

    xBee.register_receive_cb(&NewFrameReceivedHandler);
    
    RadioStatus radioStatus = xBee.init();
    MBED_ASSERT(radioStatus == Success);
    uint64_t Adr64Bits = xBee.get_addr64();
    uint32_t highAdr = Adr64Bits >> 32;
    uint32_t lowAdr = Adr64Bits;
    DEBUG_PRINTXYZNL(DEBUG, "Primary initialization successful, device 64bit Adress = '0x%X%X'", highAdr, lowAdr);

    // maybe no need anymore with node_ID
    radioStatus = xBee.set_panid(panID);
    MBED_ASSERT(radioStatus == Success);
    DEBUG_PRINTXYNL(DEBUG, "Device PanID was set to '0x%X'", panID);

    radioStatus = xBee.set_node_identifier(nodeIdentifier);
    MBED_ASSERT(radioStatus == Success);   
    DEBUG_PRINTXYNL(DEBUG, "Device node identifier was set to '%s'", nodeIdentifier);

    DEBUG_PRINTXY(DEBUG, "Waiting for device to create the network '0x%X'", panID);
    while (!xBee.is_joined()) {
        wait_ms(1000);
        DEBUG_PRINTX(DEBUG, ".");
    }
    DEBUG_PRINTXYNL(DEBUG, "\r\ndevice created network with panID '0x%X' successfully!", panID);

    /* Maybe drop this config if we dont use discovery feature...
    xBee.register_node_discovery_cb(&discovery_function);
    radioStatus = xBee.config_node_discovery(NODE_DISCOVERY_TIMEOUT_MS);
    MBED_ASSERT(radioStatus == Success);
    DEBUG_PRINTXYNL(DEBUG, "Configuration of node discovery successful with timeout at '%ims'", NODE_DISCOVERY_TIMEOUT_MS);
    */

    DEBUG_PRINTXNL(DEBUG, "XBee initialization finished successfully!");
    DEBUG_PRINTXNL(DEBUG, "\n\r");
}

void NewFrameReceivedHandler(const RemoteXBeeZB& remoteNode, bool broadcast, const uint8_t *const data, uint16_t len)
{
    uint64_t remote64Adress = remoteNode.get_addr64();
    uint32_t highAdr = remote64Adress >> 32;
    uint32_t lowAdr = remote64Adress;
    DEBUG_PRINTXYZNL(DEBUG, "\r\nGot a %s RX packet of length '%d'", broadcast ? "BROADCAST" : "UNICAST", len);
    DEBUG_PRINTXYZNL(DEBUG, "16 bit remote address is: '0x%X' and it is '%s'", remoteNode.get_addr16(), remoteNode.is_valid_addr16b() ? "valid" : "invalid");
    DEBUG_PRINTXYZ(DEBUG, "64 bit remote address is:  '0x%X%X'", highAdr, lowAdr);
    DEBUG_PRINTXYNL(DEBUG, "and it is '%s'", remoteNode.is_valid_addr64b() ? "valid" : "invalid");

    DEBUG_PRINTX(DEBUG, "Data is: ");
    for (int i = 0; i < len; i++)
        DEBUG_PRINTXY(DEBUG, "0x%X ", data[i]);

    // TODO process received data here!

    DEBUG_PRINTXNL(DEBUG, "\r\n");
}

void ChangeRemoteWaterPumpStateById(char *nodeId, bool state) {
    DEBUG_PRINTXYZNL(DEBUG, "\r\nPutting node '%s' water pump state to '%s'", nodeId, state ? "True" : "False");

    RemoteXBeeZB remoteNode = xBee.get_remote_node_by_id(nodeId);
    if(remoteNode.is_valid()) {
        /*
        uint64_t remote64 = remoteNode.get_addr64();
        uint32_t highAdr = remote64 >> 32;
        uint32_t lowAdr = remote64;*/
        char message[] = {ALTERNATE_WATER_PUMP_STATE};
        TxStatus txStatus = xBee.send_data(remoteNode, (const uint8_t *)message, 1);
        if (txStatus == TxStatusSuccess) {
            DEBUG_PRINTXYNL(DEBUG, "Pump Message was sent to '%s' successfully!\r\n", nodeId);
        } else {
            DEBUG_PRINTXYZNL(DEBUG, "Failed to send Pump Message to '%s' with error '%d', look in XBee.h for more details\r\n", nodeId, (int) txStatus);
        }
    } else {
        DEBUG_PRINTXYNL(DEBUG, "Did not find device with node Id '%s'\r\n", nodeId);
    }
}

void ChangeRemoteWaterPumpBy64BitAdr(uint64_t remote64BitsAdr, bool state) {
    DEBUG_PRINTXY(DEBUG, "\r\nPutting node with address '0x%X' water pump state to ", remote64BitsAdr);
    DEBUG_PRINTXYNL(DEBUG, "'%s'", state ? "True" : "False");

    RemoteXBeeZB remoteNode = RemoteXBeeZB(remote64BitsAdr);
    if(remoteNode.is_valid()) {
        char message[] = {ALTERNATE_WATER_PUMP_STATE};
        TxStatus txStatus = xBee.send_data(remoteNode, (const uint8_t *)message, 1);
        if (txStatus == TxStatusSuccess) {
            DEBUG_PRINTXNL(DEBUG, "Pump Message was sent to node successfully!\r\n");
        } else {
            DEBUG_PRINTXYNL(DEBUG, "Failed to send Pump Message to node with error '%d', look in XBee.h for more details\r\n", (int) txStatus);
        }
    } else {
        DEBUG_PRINTXYNL(DEBUG, "Did not find device with address '0x%X'\r\n", remote64BitsAdr);
    }
}

void CheckIfNewFrameIsPresent(void) {
    xBee.process_rx_frames();
}

void SetupEthernet() {
    DEBUG_PRINTXNL(DEBUG, "\r\nSetting up Ethernet...");
    net.init();
    net.connect();
    const char *ip = net.getIPAddress();
    DEBUG_PRINTXY(DEBUG, "IP address is: %s\r\n", ip ? ip : "No IP");
    DEBUG_PRINTXNL(DEBUG, "Ethernet setup finished successfully!\r\n");
}

void TestHTTPPost() {
    char* body = "{\"temperature\":\"10.00\",\"humidity\":\"10.00\",\"luminosity\":\"10.00\",\"water_level\":\"10.00\",\"pot_identifier\":\"7ee45d53-df2a-40e8-a2b3-c32ca07348c0\"}";
    // use HTTP map?
    //api::post("timeseries/", body, placeIdentifier);
}

void GetOperations() {
    DEBUG_PRINTXNL(DEBUG, "Get operations thread started.");
    while(true) {
        char HTTPresponse[HTTP_RESPONSE_LENGTH];
        api::get("operations/?completed=0", placeIdentifier, operations);
        //DEBUG_PRINTXNL(DEBUG, "Get Finished");
        //DEBUG_PRINTXYNL(DEBUG, "Get operations response is %s", HTTPresponse);
        operationsParser();
        Thread::wait(5000);
    }
}

void operationsParser() {
    for(int i = 0; i < api::maxOperationsLength; i++) {
        if(!strcmp(operations[i].id, "") == 0) {
            DEBUG_PRINTXYNL(DEBUG, "sending operation %d...", i);
            // send to XBee with ID
        }
    }
}

int main() {
    DEBUG_PRINTXNL(DEBUG, "Station node started");
    SetLedTo(0, true); // Init LED on

    ReadConfigFile(&panID);
    SetupEthernet();
    //SetupXBee(panID);
    //flashItTimer.start(1000);

    SetLedTo(0, false); // Init LED off
    Thread getOperationsThread(GetOperations, osPriorityNormal, 5096);
    Thread flashLed3Thread;
    flashLed3Thread.start(FlashLed3);
    getOperationsThread.start(GetOperations);

    //TestHTTPPost();

    while (true) {
        Thread::wait(osWaitForever);
    }
}

