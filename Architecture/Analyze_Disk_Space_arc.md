# analyDiskSpace() Function Documentation

## Overview

The `analyDiskSpace()` function analyzes a selected drive and displays:

- basic disk information (name, size, type, mountpoint)  
- human‑readable size conversion  
- used, available, and percentage‑used space (if mounted)  

It uses `lsblk` for hardware information and `df` for filesystem usage.

------------------------------------------------------------
## Step‑by‑Step Breakdown

### 1. Drive Selection

The user selects a drive via the TUI:

    ```cpp
    std::string drive_name = listDrives(true);
    ```

If the user cancels, the function exits early.

------------------------------------------------------------
## 2. Display Header

A simple header is printed:

    ```cpp
    std::cout << "------ Disk Information ------\n";
    ```

------------------------------------------------------------
## 3. Query Disk Information

The function runs:

    ```cpp
    lsblk -b -o NAME,SIZE,TYPE,MOUNTPOINT -n -p <drive>
    ```

This returns:

- device name  
- size in bytes  
- type (disk/part)  
- mountpoint  

The output is parsed line‑by‑line.

------------------------------------------------------------
## 4. Parse and Display Disk Details

Each line is parsed:

    ```cpp
    lss >> name >> size >> type;
    ```

Mountpoint is extracted from the remainder of the line.

Only entries where `type == "disk"` are processed.

### Human‑Readable Size Conversion

The function converts bytes → KB/MB/GB/TB:

    ```cpp
    while (human_size >= 1024 && unit < 4) {
        human_size /= 1024;
        ++unit;
    }
    ```

If conversion fails, raw bytes are printed.

### Printed fields:

- Device  
- Size (human readable)  
- Type  
- Mountpoint  

If no disk entry is found, the function prints:

    No disk info found!

------------------------------------------------------------
## 5. Filesystem Usage (if mounted)

If the disk has a valid mountpoint:

    ```cpp
    df -h '<mountpoint>' | tail -1
    ```

The function extracts:

- used  
- available  
- used percentage  

These values are printed:

    Used:  
    Available:  
    Used %:  

If the disk is **not mounted**, the function prints:

    No mountpoint, cannot show used/free space.

------------------------------------------------------------
## 6. Footer

A closing separator is printed:

    ```cpp
    std::cout << "------------------------------\n";
    ```

------------------------------------------------------------
## Summary

The `analyDiskSpace()` function provides:

- interactive drive selection  
- detailed disk information via `lsblk`  
- human‑readable size formatting  
- filesystem usage via `df`  
- graceful handling of unmounted drives  
- clear, structured output  

It is a lightweight but effective disk analysis tool integrated into DriveMgr.

------------------------------------------------------------
