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
// EventQueue eventQueue(32 * EVENTS_EVENT_SIZE); // holds 32 events
// Thread eventQueueThread;
XBeeZB xBee = XBeeZB(p13, p14, p8, NC, NC, XBEE_BAUD_RATE);
EthernetInterface net;

void SetLedTo(uint16_t led, bool state) {
    LEDs[led] = state;
}

void FlashLed(uint16_t led) {
    LEDs[led] = !LEDs[led];
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
    DEBUG_PRINTXNL(DEBUG, "\r\nreading configuration file...");

    char configurationValue[BUFSIZ];
    cfg.read(CFG_PATH);

    cfg.getValue(CFG_KEY_PAN_ID, configurationValue, BUFSIZ);
    *panID = strtol(configurationValue, NULL, HEXA_BASE);
    DEBUG_PRINTXYNL(DEBUG, "Pan ID: '0x%X'", *panID);

    cfg.getValue(CFG_KEY_NODE_IDENTIFIER, configurationValue, BUFSIZ);
    int i = 0;
    while(configurationValue[i] != '\0') {
        placeIdentifier[i] = configurationValue[i];
        i++;
    }
    DEBUG_PRINTXYNL(DEBUG, "Place identifier: '%s'", placeIdentifier);

    cfg.getValue(CFG_KEY_NODE_IDENTIFIER, configurationValue, BUFSIZ);
    while(configurationValue[i] != '\0') {
        nodeIdentifier[i] = configurationValue[i];
        i++;
    }
    DEBUG_PRINTXYNL(DEBUG, "node Identifier: '%s'", nodeIdentifier);

    DEBUG_PRINTXNL(DEBUG, "Reading configuration file finished.\r\n");
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
    DEBUG_PRINTXYNL(DEBUG, "\r\ndevice created network with panID '0x%X' successfully!\r\n", panID);

    /* Maybe drop this config if we dont use discovery feature...
    xBee.register_node_discovery_cb(&discovery_function);
    radioStatus = xBee.config_node_discovery(NODE_DISCOVERY_TIMEOUT_MS);
    MBED_ASSERT(radioStatus == Success);
    DEBUG_PRINTXYNL(DEBUG, "Configuration of node discovery successful with timeout at '%ims'", NODE_DISCOVERY_TIMEOUT_MS);
    */

    DEBUG_PRINTXNL(DEBUG, "XBee initialization finished successfully!");
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

// Not sure if it works fully, need more testing, maybe more XBees in the network?
void discovery_function(const RemoteXBeeZB& remoteNode, char const * const node_id)
{
    const uint64_t remote64Adress = remoteNode.get_addr64();
    DEBUG_PRINTXYNL(DEBUG, "HEY, I HAVE FOUND A DEVICE! node ID = '%s'", node_id);
    DEBUG_PRINTXYZNL(DEBUG, "16 bit remote address is: '0x%X' and it is '%s'", remoteNode.get_addr16(), remoteNode.is_valid_addr16b() ? "valid" : "invalid");
    DEBUG_PRINTXYZ(DEBUG, "64 bit address is:  High = '0x%X' Low = '0x%X'", UINT64_HI32(remote64Adress), UINT64_LO32(remote64Adress));
    DEBUG_PRINTXYNL(DEBUG, "and it is '%s'", remoteNode.is_valid_addr64b() ? "valid" : "invalid");

    if (remoteNodesCount < MAX_NODES) {
        remoteNodesInNetwork[remoteNodesCount] = remoteNode;
        remoteNodesCount++;
    } else {
        DEBUG_PRINTXYNL(DEBUG, "Found more nodes than maximum configured, max is '%i'", MAX_NODES);
    } 
}

// Not sure if it works fully, need more testing, maybe more XBees in the network?
void DiscoverAllNodes(void) {
    DEBUG_PRINTX(DEBUG, "Discovering all nodes process started");
    xBee.start_node_discovery();
    do {
        xBee.process_rx_frames();
        wait_ms(10);
        DEBUG_PRINTX(DEBUG, ".");
    } while(xBee.is_node_discovery_in_progress());
    DEBUG_PRINTXNL(DEBUG, "Discovering all nodes process finished!");
}

// Works but not sure if will use...
void DiscoverNodeById(char *nodeId) {
    DEBUG_PRINTXYNL(DEBUG, "Discovering by Id process started, trying to find node Id '%s'", nodeId);
    RemoteXBeeZB remoteNode = xBee.get_remote_node_by_id(nodeId);

    if(remoteNode.is_valid()) {
        const uint64_t remote64Adress = remoteNode.get_addr64();
        DEBUG_PRINTXYNL(DEBUG, "Found '%s'!", nodeId);
        DEBUG_PRINTXYZNL(DEBUG, "16 bits remote address is: '0x%X' and it is '%s'", remoteNode.get_addr16(), remoteNode.is_valid_addr16b() ? "valid" : "invalid");
        DEBUG_PRINTXYZ(DEBUG, "64 bits address is:  High = '0x%X' Low = '0x%X'", UINT64_HI32(remote64Adress), UINT64_LO32(remote64Adress));
        DEBUG_PRINTXYNL(DEBUG, "and it is '%s'", remoteNode.is_valid_addr64b() ? "valid" : "invalid");
    } else {
        DEBUG_PRINTXYNL(DEBUG, "Did not find device with node Id '%s'", nodeId);
    }
    DEBUG_PRINTXNL(DEBUG, "Discovering by Id process finished!\r\n");
}

void ChangeRemoteWaterPumpStateById(char *nodeId, bool state) {
    DEBUG_PRINTXYZNL(DEBUG, "\r\nPutting node '%s' water pump state to '%s'", nodeId, state ? "True" : "False");

    RemoteXBeeZB remoteNode = xBee.get_remote_node_by_id(nodeId);
    if(remoteNode.is_valid()) {
        uint64_t remote64 = remoteNode.get_addr64();
        uint32_t highAdr = remote64 >> 32;
        uint32_t lowAdr = remote64;
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

void StartEventQueueThread(void) {
    DEBUG_PRINTXNL(DEBUG, "Starting event queue thread");

    // eventQueue.call_every(1000, FlashLed, 3);  // just so we can see the event queue still runs
    // eventQueue.call_every(100, CheckIfNewFrameIsPresent);
    // eventQueue.call_every(5000, ChangeRemoteWaterPumpStateById, (char*)POT_1, true);
    // eventQueue.call_every(2000, ChangeRemoteWaterPumpBy64BitAdr, 0x0013A20040331988, true); // hardcoded 64 adr value for now
    // eventQueueThread.start(callback(&eventQueue, &EventQueue::dispatch_forever));

    DEBUG_PRINTXNL(DEBUG, "Event thread queue started sucessfully.");
}

void SetupEthernet() {
    net.init();
    net.connect();
    const char *ip = net.getIPAddress();
    DEBUG_PRINTXY(DEBUG, "IP address is: %s\r\n", ip ? ip : "No IP");
}

void ParseOperations(const yajl_val &node) {
    const char * path[] = { "results", (const char *) 0 };
    yajl_val results = yajl_tree_get( node, path, yajl_t_array );
    if ( results && YAJL_IS_ARRAY(results) ) {
        size_t len = results->u.array.len;
        for (int i = 0; i < len; ++i ) {
            const char * actionPath[] = { "action", (const char *) 0 };
            yajl_val action = yajl_tree_get(results->u.array.values[i], actionPath, yajl_t_string);

            const char * identifierPath[] = { "pot_identifier", (const char *) 0 };
            yajl_val pot_identifier = yajl_tree_get(results->u.array.values[i], identifierPath, yajl_t_string);
            if (action && pot_identifier) {
                // Here we have an action and a pot_identifier
                DEBUG_PRINTXYZ(DEBUG, "pot_identifer: %s\taction: %s\r\n", YAJL_GET_STRING(pot_identifier), YAJL_GET_STRING(action));
            } else {
                DEBUG_PRINTX(DEBUG, "no action or pot_identifier\r\n");
            }
        }
    } else { 
        DEBUG_PRINTX(DEBUG, "No actions to do\r\n" ); 
    }

    yajl_tree_free(node);
}

void TestHTTPPost() {
    char* body = "{\"temperature\":\"10.00\",\"humidity\":\"10.00\",\"luminosity\":\"10.00\",\"water_level\":\"10.00\",\"pot_identifier\":\"7ee45d53-df2a-40e8-a2b3-c32ca07348c0\"}";
    api::post("timeseries/", body, placeIdentifier);
}

void TestHTTPGET() {
    yajl_val node = api::get("operations/", placeIdentifier);
}

int main() {
    DEBUG_PRINTXNL(DEBUG, "Station node started");
    SetLedTo(0, true); // Init LED on

    GetMacAddress(macAdr);
    ReadConfigFile(&panID);
    SetupEthernet();
    // SetupXBee(panID);

    StartEventQueueThread();
    SetLedTo(0, false); // Init LED off

    TestHTTPGET();
    TestHTTPPost();

    while (true) {
        Thread::wait(osWaitForever);
    }
}

