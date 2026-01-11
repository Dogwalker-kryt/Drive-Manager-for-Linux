# formatDrive() Function Documentation

## Overview

The `formatDrive()` function provides a multi‑mode interface for formatting drives.  
It supports three formatting modes:

1. **Format drive (default ext4)**  
2. **Format drive with a custom label**  
3. **Format drive with a custom label and filesystem type**

Each mode includes:

- interactive drive selection  
- user confirmation  
- execution of the appropriate `mkfs` command  
- error handling and logging  

This function ensures that formatting is always intentional and traceable.

------------------------------------------------------------
## Menu Selection

The function begins by displaying a formatting mode menu:

    ```cpp
    std::cout << "1. Format drive\n";
    std::cout << "2. Format drive with label\n";
    std::cout << "3. Format drive with label and filesystem\n";
    ```

The user selects a mode:

    ```cpp
    int fdinput;
    std::cin >> fdinput;
    ```

A `switch` statement dispatches the selected formatting mode.

------------------------------------------------------------
# Case 1 — Basic Formatting (ext4)

    ```cpp
    case 1:
    ```

### Steps:

1. **Drive selection** via TUI:

        std::string driveName = listDrives(true);

2. **User confirmation**:

        Are you sure you want to format drive <drive>? (y/n)

3. If confirmed, run:

        mkfs.ext4 <driveName>

4. **Check for errors**:

        if (result.find("error") != npos)

5. Log success or failure.

If the user cancels, the function logs the cancellation and returns.

------------------------------------------------------------
# Case 2 — Formatting with Label

    ```cpp
    case 2:
    ```

### Steps:

1. **Drive selection**:

        std::string driveName = listDrives(true);

2. **Ask for label**:

        std::cin >> label;

3. Reject empty labels:

        if (label.empty()) { error }

4. **User confirmation**:

        Format drive <drive> with label '<label>'? (y/n)

5. Execute:

        mkfs.ext4 -L <label> <driveName>

6. Detect errors and log accordingly.

------------------------------------------------------------
# Case 3 — Formatting with Label + Filesystem Type

    ```cpp
    case 3:
    ```

### Steps:

1. **Drive selection**:

        std::string driveName = listDrives(true);

2. **Ask for label and filesystem type**:

        std::cin >> label;
        std::cin >> fsType;

3. **User confirmation**:

        Format <drive> with label '<label>' and filesystem '<fsType>'? (y/n)

4. Execute:

        mkfs.<fsType> -L <label> <driveName>

5. Detect errors and log success or failure.

This mode allows formatting with any filesystem supported by `mkfs.*` (e.g., ext4, xfs, btrfs).

------------------------------------------------------------
# Default Case — Invalid Input

If the user enters an invalid menu option:

    ```cpp
    std::cerr << "[Error] Invalid option selected.\n";
    ```

The function exits without performing any formatting.

------------------------------------------------------------
# Summary

The `formatDrive()` function provides a safe and flexible interface for drive formatting.  
It includes:

- interactive drive selection  
- three formatting modes  
- label and filesystem customization  
- confirmation prompts  
- robust error detection  
- detailed logging  

This ensures that destructive operations like formatting are always deliberate and properly recorded.

------------------------------------------------------------
