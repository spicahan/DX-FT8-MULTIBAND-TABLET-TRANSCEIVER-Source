This repo hosts my changes on top of the official DX-FT8 firmware hosted at https://github.com/g8kig/DX-FT8-MULTIBAND-TABLET-TRANSCEIVER-Source/

If you're only interested in using the pre-built firmware, just check out the releases: https://github.com/spicahan/DX-FT8-MULTIBAND-TABLET-TRANSCEIVER-Source/releases

If you've already used the official firmware, you should be able to use my firwmare right away. Hopefully it's intuitive enough. The biggest difference you would notice is that when you click on a decoded message, the transmission starts immediately. There are a bunch of other changes of reliability and useability improvements. Please refer to the release notes in https://github.com/spicahan/DX-FT8-MULTIBAND-TABLET-TRANSCEIVER-Source/releases for the full list of features.

If you're interested in the source code and development, all of my changes are in the branch "autoseq". Note that I use VSCode + CMake + the latest ARM toolchain for developement. I use the virtual USB storage device provided by STLink for flashing the firmware. I have gdb/lldb integration with VSCode for live debugging. I also developed a bunch of host side HAL mocks and a simulator that you can run on host to test the behavior of the code. It can accurately simulate up to 1ms. Free feel to join https://dxft8.groups.io/g/EXPERIMENTER/ to discuss more.

Below are the original README.md from https://github.com/g8kig/DX-FT8-MULTIBAND-TABLET-TRANSCEIVER-Source/

For more information about the DX FT8 Multiband Tablet Transceiver please use the

https://github.com/WB2CBA/DX-FT8-FT8-MULTIBAND-TABLET-TRANSCEIVER

repository.

The original sources for the transceiver firmware are stored on a shared google drive (called Katy.zip)

I was unable to build these sources. 
I held an email exchange with Charles Hill, W5BAA. 
Charley supplied me with a very useful PDF to get the sources building using the STM32 Cube IDE. 
I am using the STM32CubeIDE Version: 1.16.1

Unfortunately I was still unable to build the sources due to build errors.
These errors were due to C variables being defined in header files causing multiple definition errors at link time.
I suspect in part this is because Arduino-style C++ code has been converted to C.

The source code contains the sources to the excellent FT8 library by Karlis Goba, YL3JG.
The original repo for the ft8 library is at

https://github.com/kgoba/ft8_lib

I modified the sources to build the firmware successfully.

Paul G8KIG
