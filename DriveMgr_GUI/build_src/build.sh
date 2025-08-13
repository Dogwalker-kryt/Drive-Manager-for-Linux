#!/bin/bash

# Get GTK4 compilation flags
GTK_CFLAGS=$(pkg-config --cflags gtk4)
GTK_LIBS=$(pkg-config --libs gtk4)

# Compile the source files
g++ -c src/functions.cpp -I. -o src/functions.o
g++ -c src/main.cpp -I. $GTK_CFLAGS -o src/main.o

# Link everything together
g++ src/functions.o src/main.o -o DriveMgr-GUI \
    -lssl -lcrypto $GTK_LIBS

# Make executable
chmod +x DriveMgr-GUI
