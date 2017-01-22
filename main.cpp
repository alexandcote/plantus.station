#include "mbed.h"

Serial pc(USBTX, USBRX);
DigitalOut led1(LED1);

// main() runs in its own thread in the OS
// (note the calls to wait below for delays)
int main() {
    while (true) {
        pc.getc();
        led1 = !led1;
        wait(0.1);
        led1 = !led1;
    }
}

