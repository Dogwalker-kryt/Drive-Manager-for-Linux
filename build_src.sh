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
cd ~/Drive-Manager-for-Linux
cd ~/DriveMgr-Manager-for-Linux/DriveMgr_CLI/src
g++ DriveMgr_experi.cpp -I.. -o DriveMgr_experi -lssl -lcrypto 
g++ DriveMgr_stable.cpp -I.. -o DriveMgr_stable -lssl -lcrypto 
cd ~/DriveMgr-Manager-for-Linux/DriveMgr_GUI/src
g++ DriveMgr_GUI.cpp -I.. -o DriveMgr_GUI -lssl -lcrypto `pkg-config --cflags --libs gtk4`
mkdir -p ~/Drive-Manager-for-Linux/bin
find ~/DriveMgr-Manager-for-Linux -executable -exec mv {} ~/Drive-Manager-for-Linux/bin \;
echo "Build completed successfully. Executables are located in ~/Drive-Manager-for-Linux/bin."
echo "Creating  ~/-var/app/DriveMgr directrory"
cd ~/.var/app
mkdir -p DriveMgr
cd DriveMgr
mkdir -p bin
cp ~/DriveMgr/bin/* ~/.var/app/DriveMgr/bin/
touch ~/.var/app/DriveMgr/log.txt
touch ~/.var/app/DriveMgr/keys.savekey
echo "Executables copied to ~/.var/app/DriveMgr/bin/"
echo "log and keys files created in ~/.var/app/DriveMgr/"
exit 0
