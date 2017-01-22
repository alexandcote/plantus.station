# Plantus-Station

Install GCC for ARM
```
brew cask install gcc-arm-embedded
```

Installing mbed-cli
```
pip install mbed-cli
```

Clonning the repository
```
mbed import git@github.com:alexandcote/plantus-station.git plantus-station
```

Compile the project
```
mbed compile
```

Run the script to copy the binary file on the MBED
```
./mbed_copy BUILD/LPC1768/GCC_ARM/plantus-station.bin /Volumes/MBED
```
