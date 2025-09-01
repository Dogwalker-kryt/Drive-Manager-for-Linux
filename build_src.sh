#!/bin/bash

if ! g++ --version > /dev/null 2>&1; then
    echo "g++ is not installed"
    NEED_INSTALL=1
fi

if ! openssl version > /dev/null 2>&1; then
    echo "openssl is not installed"
    NEED_INSTALL=1
fi

if ! pkg-config --exists libgtk-4-dev; then
    echo "libgtk-4-dev is not installed"
    NEED_INSTALL=1
fi

if ! smartctl --version > /dev/null 2>&1; then
    echo "smart tools are not installed"
    NEED_INSTALL=1
fi

if [ "${NEED_INSTALL}" = "1" ]; then
    echo "Some required packages are not installed."
    echo "Do you want to install the required packages? [y/n]"
    read -r install_choice
    if [ "$install_choice" = "y" ]; then
        sudo apt update
        sudo apt install openssl
        sudo apt-get libssl
        sudo apt install libgtk-4-dev
        sudo apt install g++
        sudo apt install smartmontools
    fi
fi
PROJECT_ROOT="$HOME/Drive-Manager-for-Linux"
mkdir -p "$PROJECT_ROOT/bin"

cd "$PROJECT_ROOT/DriveMgr_CLI/src" || exit 1
g++ DriveMgr_experi.cpp -I.. -o DriveMgr_experi -lssl -lcrypto || exit 1
g++ DriveMgr_stable.cpp -I.. -o DriveMgr_stable -lssl -lcrypto || exit 1

cd "$PROJECT_ROOT/DriveMgr_GUI/build_src" || exit 1
bash build.sh
# g++ DriveMgr_GUI.cpp -I.. -o DriveMgr_GUI -lssl -lcrypto $(pkg-config --cflags --libs gtk4) || exit 1

find . -type f -executable -exec mv {} "$PROJECT_ROOT/bin/" \;
echo "Build completed successfully. Executables are located in ~/Drive-Manager-for-Linux/bin."
APP_DIR="$HOME/.var/app/DriveMgr"
echo "Creating application directory at $APP_DIR"
mkdir -p "$APP_DIR/bin"

if [ -d "$PROJECT_ROOT/bin" ] && [ "$(ls -A "$PROJECT_ROOT/bin")" ]; then
    cp "$PROJECT_ROOT/bin"/* "$APP_DIR/bin/" || {
        echo "Error: Failed to copy executables"
        exit 1
    }
    echo "Executables copied to $APP_DIR/bin/"
else
    echo "Error: No executables found in $PROJECT_ROOT/bin"
    exit 1
fi

touch "$APP_DIR/log.txt"
touch "$APP_DIR/keys.savekey"
echo "Log and keys files created in $APP_DIR"

echo "Build and setup completed successfully!"
exit 0
