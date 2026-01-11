# resizeDrive() Function Documentation

## Overview

The `resizeDrive()` function allows the user to resize a drive’s primary partition using the `parted` utility.  
It provides:

- interactive drive selection  
- user‑defined new size (in GB)  
- validation of input  
- execution of a safe, scripted `parted` resize command  
- error handling and logging  

This function modifies **partition 1** of the selected drive.

------------------------------------------------------------

## Step‑by‑Step Breakdown

### 1. Drive Selection

The user selects a drive using the TUI selector:

    ```cpp
    std::string driveName = listDrives(true);
    ```

If the user cancels or no drive is selected, the function returns.

------------------------------------------------------------

### 2. Ask for New Size

The user is prompted to enter the new size in gigabytes:

    ```cpp
    std::cout << "Enter new size in GB for drive " << driveName << ":\n";
    int new_size;
    std::cin >> new_size;
    ```

Invalid sizes (≤ 0) are rejected:

    ```cpp
    if (new_size <= 0) {
        std::cout << "[Error] Invalid size entered!\n";
        return;
    }
    ```

------------------------------------------------------------

### 3. Inform User About Operation

A status message is printed:

    ```cpp
    std::cout << "Resizing drive " << driveName << " to " << new_size << " GB...\n";
    ```

------------------------------------------------------------

### 4. Execute the Resize Command

The function constructs a `parted` command:

    ```cpp
    std::string resize_cmd =
        "sudo parted --script " + driveName +
        " resizepart 1 " + std::to_string(new_size) + "GB";
    ```

This command:

- runs non‑interactively (`--script`)
- resizes **partition 1**
- sets the new end boundary to `<new_size>GB`

The command is executed:

    ```cpp
    std::string resize_output = runTerminal(resize_cmd);
    ```

The output is printed:

    ```cpp
    std::cout << resize_output << "\n";
    ```

------------------------------------------------------------

### 5. Error Detection

If the output contains `"error"`:

    ```cpp
    if (resize_output.find("error") != std::string::npos) {
        std::cout << RED << "[Error] Failed to resize drive\n";
        Logger::log("[ERROR] Failed to resize drive: " + driveName);
    }
    ```

Otherwise, the operation is considered successful:

    ```cpp
    else {
        std::cout << GREEN << "Drive resized successfully\n";
        Logger::log("[INFO] Drive resized successfully: " + driveName);
    }
    ```

------------------------------------------------------------

### 6. Exception Handling

Any unexpected runtime errors are caught:

    ```cpp
    catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception during resize: " << e.what();
        Logger::log("[ERROR] Exception during resize: " + e.what());
    }
    ```

This prevents the program from crashing and ensures the error is logged.

------------------------------------------------------------

## Summary

The `resizeDrive()` function:

- lets the user select a drive interactively  
- validates the new size  
- uses `parted` to resize partition 1  
- prints the command output  
- logs success or failure  
- handles exceptions gracefully  

It provides a simple and safe interface for resizing partitions through DriveMgr.

------------------------------------------------------------
