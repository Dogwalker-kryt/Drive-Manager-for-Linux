#!/bin/bash

what_needs_to_be_installed=()

# checks if the needed things are installed
if ! rustc --version > /dev/null 2>&1; then
    echo "Rust compiler is not installed"
    NEED_INSTALL_RUST=1
fi

if ! g++ --version > /dev/null 2>&1; then
    echo "g++ is not installed"
    NEED_INSTALL_CPP=1
fi

if ! openssl version > /dev/null 2>&1; then
    echo "openssl is not installed"
    NEED_INSTALL_OPENSSL=1
fi

if ! smartctl --version > /dev/null 2>&1; then
    echo "smart tools are not installed"
    NEED_INSTALL_SMARTTOOLS=1
fi

# checks if the value is 1, if its one then it will add it to a list 
if [ "$NEED_INSTALL_RUST" = "1" ]; then
    what_needs_to_be_installed+=("Rust_compiler")
fi

if [ "$NEED_INSTALL_CPP" = "1" ]; then
    what_needs_to_be_installed+=("g++_compiler")
fi

if [ "$NEED_INSTALL_OPENSSL" = "1" ]; then
    what_needs_to_be_installed+=("Openssl")
fi

if [ "$NEED_INSTALL_SMARTTOOLS" = "1" ]; then
    what_needs_to_be_installed+=("Smarttools")
fi

# this will read teh list with the things to install, asks for installtion and installs everything
if [ "${#what_needs_to_be_installed[@]}" -ne 0 ]; then
    echo "These are all required packages/tools that need to be installed:"
    echo "${what_needs_to_be_installed[@]}"
    echo "Do you want to install them? (y/n)"
    read -r install_choice
    if [ "$install_choice" = "y" ]; then
        for item in "${what_needs_to_be_installed[@]}"; do
            case "$item" in
                "Rust_compiler")
                    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
                    source "$HOME/.cargo/env"
                    ;;
                "g++_compiler")
                    sudo apt update
                    sudo apt install -y g++
                    ;;
                "Openssl")
                    sudo apt install -y openssl libssl-dev
                    ;;
                "Smarttools")
                    sudo apt install -y smartmontools
                    ;;
            esac
        done
    fi
fi

# the initial compiling/building process
PROJECT_ROOT="$HOME/Drive-Manager-for-Linux"
mkdir -p "$PROJECT_ROOT/bin"

cd "$PROJECT_ROOT/DriveMgr_CLI/src" || exit 1
g++ DriveMgr_experi.cpp -I.. -o DriveMgr_experi -lssl -lcrypto || exit 1
g++ DriveMgr_stable.cpp -I.. -o DriveMgr_stable -lssl -lcrypto || exit 1

cd "$PROJECT_ROOT/DriveMgr_GUI" || exit 1
if command -v cargo > /dev/null 2>&1; then
    echo "Building Rust GUI using cargo..."
    cargo build --release || {
        echo "Error: Failed to build Rust GUI"
        exit 1
    }
    RUST_GUI_BINARY="$PROJECT_ROOT/DriveMgr_GUI/target/release/drive_mgr_gui"
    if [ -f "$RUST_GUI_BINARY" ]; then
        cp "$RUST_GUI_BINARY" "$PROJECT_ROOT/bin/"
        echo "Rust GUI binary copied to bin/"
    else
        echo "Error: Rust GUI binary not found!"
        exit 1
    fi
else
    echo "Error: cargo is not installed. Skipping Rust GUI build."
fi

cd "$PROJECT_ROOT/DriveMgr_GUI/build_src" 2>/dev/null && bash build.sh

find "$PROJECT_ROOT" -type f -executable -path "$PROJECT_ROOT/DriveMgr_GUI/build_src/*" -exec mv {} "$PROJECT_ROOT/bin/" \;
echo "Build completed. Binaries moved to ~/Drive-Manager-for-Linux/bin"


APP_DIR="$HOME/.var/app/DriveMgr"
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
touch "$APP_DIR/log.dat"
touch "$APP_DIR/keys.bin"
echo "Log and keys files created in $APP_DIR"
echo "Build and setup completed successfully!"

# ask if you wnt to uninstall all teh things the porgram instlled, it will  only uninstall these ones that were in the list
echo "Do you want to uninstall everything the script installed? (y/n)"
read -r uninstallques
if [ "$uninstallques" = "y" ]; then
    for item in "${what_needs_to_be_installed[@]}"; do
        case "$item" in
            "Rust_compiler")
                rustup self uninstall -y
                ;;
            "g++_compiler")
                sudo apt remove -y g++
                ;;
            "Openssl")
                sudo apt remove -y openssl libssl-dev
                ;;
            "Smarttools")
                sudo apt remove -y smartmontools
                ;;
        esac
    done
    echo "Uninstallation completed."
fi
exit 0
