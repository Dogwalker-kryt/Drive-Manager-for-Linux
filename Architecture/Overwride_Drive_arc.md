# OverwriteDriveData() Function Documentation

## Overview

The `OverwriteDriveData()` function performs a **secure destructive wipe** of an entire drive.  
It uses two overwrite passes:

1. Random data (`/dev/urandom`)
2. Zero‑fill (`/dev/zero`)

Before performing the wipe, the function includes multiple safety layers:

- interactive drive selection  
- user confirmation prompt  
- randomly generated confirmation key  
- detailed error handling  
- logging of all events  

This ensures the user cannot accidentally erase a drive without explicit intent.

------------------------------------------------------------

## Function Flow

### 1. Drive Selection

The user selects a drive using the interactive TUI:

    ```cpp
    std::string driveName = listDrives(true);
    ```

If the user cancels or no drive is selected, the function exits.

------------------------------------------------------------

### 2. First Confirmation Prompt

The user must confirm the operation:

    ```cpp
    std::cout << "Are you sure you want to overwrite the drive " << driveName << "? (y/n)\n";
    ```

If the user answers anything other than `y` or `Y`, the operation is cancelled:

    ```cpp
    std::cout << "[Info] Overwriting cancelled\n";
    Logger::log("[INFO] Overwriting cancelled");
    ```

------------------------------------------------------------

### 3. Confirmation Key Challenge

To prevent accidental destruction, a random confirmation key is generated:

    ```cpp
    std::string Key = confirmationKeyGenerator();
    ```

The user must re‑enter this key:

    ```cpp
    if (random_confirmation_key_input != Key) {
        std::cerr << "[ERROR] Invalid confirmation\n";
        Logger::log("[ERROR] Invalid confirmation");
        return;
    }
    ```

If the key is incorrect, the wipe is aborted.

------------------------------------------------------------

## 4. Overwrite Procedure

A message is displayed:

    ```cpp
    std::cout << "Proceeding with Overwriting " << driveName << "...\n";
    ```

Two overwrite passes are executed:

### Pass 1 — Random Data

    ```cpp
    std::string devrandom =
        runTerminalV2("sudo dd if=/dev/urandom of=" + driveName +
                      " bs=1M status=progress && sync");
    ```

### Pass 2 — Zero Fill

    ```cpp
    std::string devZero =
        runTerminalV2("sudo dd if=/dev/zero of=" + driveName +
                      " bs=1M status=progress && sync");
    ```

------------------------------------------------------------

## 5. Error Handling Logic

The function checks the output of both commands:

### Both operations failed:

    ```cpp
    if (devZero.find("error") != npos && devrandom.find("error") != npos)
    ```

→ Logs error and throws exception.

### One operation failed:

    ```cpp
    else if (devZero.find("error") != npos || devrandom.find("error") != npos)
    ```

→ Warns user that only one pass succeeded.

### dd reported “failed”:

    ```cpp
    else if (devZero.find("failed") != npos || devrandom.find("failed") != npos)
    ```

→ Warns user but does not treat as fatal.

### dd output contains “dd” (ambiguous failure):

    ```cpp
    else if (devZero.find("dd") != npos || devrandom.find("dd") != npos)
    ```

→ Warns user of possible partial overwrite.

### Success:

    ```cpp
    else {
        std::cout << "Overwriting completed...\n";
    }
    ```

------------------------------------------------------------

## 6. Exception Handling

All runtime errors inside the overwrite block are caught:

    ```cpp
    catch(const std::exception& e) {
        Logger::log("[ERROR] Overwrite failed: " + e.what());
        throw std::runtime_error("[Error] Overwrite failed: " + e.what());
    }
    ```

The outer try/catch ensures initialization errors are also logged:

    ```cpp
    catch(const std::exception& e) {
        std::cerr << "[ERROR] Failed to initialize Overwriting function\n";
        Logger::log("[ERROR] Failed to initialize Overwriting function");
    }
    ```

------------------------------------------------------------

## Summary

`OverwriteDriveData()` is a **high‑risk, high‑security** function designed to safely and intentionally erase all data on a selected drive.

It includes:

- interactive drive selection  
- double confirmation (prompt + random key)  
- two‑pass overwrite (random + zero)  
- detailed error detection  
- robust exception handling  
- full logging  

This ensures the operation is both **intentional** and **traceable**, minimizing the risk of accidental data loss.

------------------------------------------------------------
