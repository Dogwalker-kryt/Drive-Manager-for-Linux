#!/bin/bash
g++ --version
openssl version
if [ $? -ne 0 ]; then
    echo "g++ and/or the build essentials are not installed. insalling g++ and/or the build-essentials"
    echo "Do you want to install g++ and th build-essentials (if not installed)? [y/n]"
    read -r install-choice
    if [ "$install-choice" = "y" ]; then
        sudo apt update
        sudo apt install openssl
        sudo apt install gtk-4.0
        sudo apt install g++
    fi
fi
cd ~/DriveMgr
cd ~/DriveMgr/DriveMgr_CLI/src
g++ DriveMgr_experi.cpp -I.. -o DriveMgr_experi -lssl -lcrypto 
g++ DriveMgr_stable.cpp -I.. -o DriveMgr_stable -lssl -lcrypto 
cd ~/DriveMgr/DriveMgr_GUI/src
g++ DriveMgr_GUI.cpp -I.. -o DriveMgr_GUI -lssl -lcrypto `pkg-config --cflags --libs gtk4`
mkdir -p ~/DriveMgr/bin
find ~/DriveMgr -executable -exec mv {} ~/DriveMgr/bin \;
echo "Build completed successfully. Executables are located in ~/DriveMgr/bin."
exit 0
