# Plantus-Station

Install GCC for ARM
```
brew cask install gcc-arm-embedded
```

Installing mbed-cli
```
sudo pip install mbed-cli
```

Clonning the repository
```
mbed import git@github.com:alexandcote/plantus.station.git plantus.station
```

Compile the project
```
cd plantus.station
mbed compile -m LPC1768 -t GCC_ARM
```

Run the script to copy the binary file on the MBED
```
./mbed_copy BUILD/LPC1768/GCC_ARM/plantus.station.bin /Volumes/MBED
```

Fix compiling errors
(https://developer.mbed.org/users/RyoheiHagimoto/notebook/compiler-error-of-ethernetinterface/)

EthernetInterface\lwip\include\lwip\sockets.h
```
#define LWIP_TIMEVAL_PRIVATE 1
     ↓
#define LWIP_TIMEVAL_PRIVATE 0
```

EthernetInterface\lwip-sys\arch\cc.h(35)
```
#include <stdint.h>
     ↓
#include <stdint.h>
#include <sys/time.h>
```
