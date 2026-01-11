# listDrives() Function Documentation

## Overview

The `listDrives()` function is responsible for:

- retrieving all physical drives using `lsblk`
- displaying them in a formatted table
- optionally enabling an **inline TUI selector** that allows the user to choose a drive using arrow keys
- returning the selected drive path when `input_mode == true`

It is used throughout DriveMgr whenever the user must select a drive interactively.

------------------------------------------------------------

## Global Selected Drive

A global variable stores the last selected drive:

    ```cpp
    std::string g_selected_drive;
    ```

------------------------------------------------------------

## Function Signature

    ```cpp
    std::string listDrives(bool input_mode)
    ```

- `input_mode == false`  
  → Only prints the drive list, returns an empty string.

- `input_mode == true`  
  → Enables TUI selection and returns the chosen drive path.

------------------------------------------------------------

## Drive Enumeration

A static vector is used to store discovered drives:

    ```cpp
    static std::vector<std::string> drives;
    drives.clear();
    ```

The function retrieves drive information using `lsblk`:

    ```cpp
    std::string lsblk =
        runTerminal("lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -d -n -p");
    ```

The `-d` flag ensures only **physical drives** are listed (no partitions).

------------------------------------------------------------

## Table Header Rendering

The function prints a formatted header for the drive list:

    ```cpp
    std::cout << CYAN << "\nAvailable Drives:" << RESET << "\n";
    std::cout << std::left
              << std::setw(2)
              << std::setw(5)  << "#"
              << BOLD << std::setw(18) << "Device"      << RESET
              << BOLD << std::setw(10) << "Size"        << RESET
              << BOLD << std::setw(10) << "Type"        << RESET
              << BOLD << std::setw(15) << "Mountpoint"  << RESET
              << BOLD << std::setw(10) << "FSType"      << RESET
              << "Status" << std::endl;
    ```

A separator line is printed:

    ```cpp
    std::cout << CYAN << std::string(90, '-') << "\n" << RESET;
    ```

------------------------------------------------------------

## Parsing `lsblk` Output

The output is parsed line-by-line:

    ```cpp
    std::istringstream iss(lsblk);
    std::string line;
    int idx = 0;
    ```

A `Row` struct stores each drive’s attributes:

    ```cpp
    struct Row {
        std::string device, size, type, mount, fstype, status;
    };
    ```

Only lines containing `"disk"` are processed:

    ```cpp
    if (line.find("disk") == std::string::npos) continue;
    ```

Each row is parsed into fields:

    ```cpp
    lss >> r.device >> r.size >> r.type;
    ```

Mountpoint and filesystem type are extracted:

    ```cpp
    rss >> r.mount >> r.fstype;
    if (r.mount == "-") r.mount = "";
    if (r.fstype == "-") r.fstype = "";
    ```

The filesystem status is determined:

    ```cpp
    r.status = checkFilesystem(r.device, r.fstype);
    ```

Each row is printed and stored:

    ```cpp
    drives.push_back(r.device);
    rows.push_back(r);
    idx++;
    ```

If no drives are found:

    ```cpp
    std::cerr << RED << "No drives found!\n" << RESET;
    return "";
    ```

------------------------------------------------------------

## TUI Selection Mode (`input_mode == true`)

When enabled, the function switches the terminal into raw mode:

    ```cpp
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ```

### Cursor Positioning

The cursor is moved upward to allow inline updates:

    ```cpp
    std::cout << "\033[" << total << "A";
    ```

### Rendering the Interactive Selector

A loop continuously redraws the table with a highlighted row:

    ```cpp
    if (i == selected)
        std::cout << SELECTION_COLOR << "> " << RESET;
    else
        std::cout << "  ";
    ```

The selected row is highlighted:

    ```cpp
    if (i == selected) std::cout << SELECTION_COLOR;
    ...
    if (i == selected) std::cout << RESET;
    ```

### Handling Arrow Keys

Arrow key escape sequences are processed:

    ```cpp
    if (c == '\x1b') {
        char seq[2];
        if (read(STDIN_FILENO, &seq, 2) == 2) {
            if (seq[1] == 'A') selected = (selected - 1 + total) % total; // up
            if (seq[1] == 'B') selected = (selected + 1) % total;         // down
        }
    }
    ```

Pressing Enter finalizes the selection:

    ```cpp
    else if (c == '\n' || c == '\r') {
        break;
    }
    ```

### Cleanup

The cursor is moved below the table:

    ```cpp
    int tableheight = total + 3;
    std::cout << "\033[" << tableheight << "B";
    ```

Terminal settings are restored:

    ```cpp
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    ```

The selected drive is stored globally and returned:

    ```cpp
    g_selected_drive = drives[selected];
    return g_selected_drive;
    ```

------------------------------------------------------------

## Summary

The `listDrives()` function:

- retrieves all physical drives using `lsblk`
- prints them in a formatted table
- optionally enables an inline TUI selector
- supports arrow‑key navigation
- returns the selected drive path when `input_mode == true`

This function is a core component of DriveMgr’s interactive workflow, providing a clean and intuitive way for users to select drives.

------------------------------------------------------------
