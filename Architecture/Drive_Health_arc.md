# checkDriveHealth() Function Documentation

## Overview

The `checkDriveHealth()` function performs a SMART health check on a selected drive using the `smartctl` utility.  
It provides:

- interactive drive selection  
- execution of a SMART health test (`smartctl -H`)  
- output display directly to the user  
- exception handling and logging  

This function is used to quickly determine whether a drive is reporting hardware issues.

------------------------------------------------------------

## Step‑by‑Step Breakdown

### 1. Drive Selection

The user selects a drive interactively:

    ```cpp
    std::string driveHealth_name = listDrives(true);
    ```

If the user cancels or no drive is selected, the function returns early.

------------------------------------------------------------

### 2. Build and Execute SMART Command

A SMART health check command is constructed:

    ```cpp
    std::string check_drive_helth_cmd =
        "sudo smartctl -H " + driveHealth_name;
    ```

The command is executed using the terminal wrapper:

    ```cpp
    std::string check_drive_helth_cmd_output =
        runTerminal(check_drive_helth_cmd);
    ```

The output is printed directly to the console:

    ```cpp
    std::cout << check_drive_helth_cmd_output;
    ```

This typically includes:

- SMART overall‑health status  
- drive readiness  
- warnings if the drive is failing  

------------------------------------------------------------

### 3. Exception Handling

Any runtime errors (e.g., missing smartctl, permission issues) are caught:

    ```cpp
    catch(const std::exception& e) {
        std::string error = e.what();
        Logger::log("[ERROR]" + error);
        throw std::runtime_error(e.what());
    }
    ```

The error is logged and re‑thrown to ensure the caller is aware of the failure.

------------------------------------------------------------

## Return Value

The function always returns:

    ```cpp
    return 0;
    ```

This indicates completion, regardless of SMART output content.

------------------------------------------------------------

## Summary

The `checkDriveHealth()` function:

- lets the user select a drive  
- runs a SMART health check using `smartctl -H`  
- prints the full output to the user  
- logs any exceptions  
- provides a simple and reliable way to check drive health  

It is a lightweight wrapper around SMART diagnostics, integrated into DriveMgr’s TUI workflow.

------------------------------------------------------------
