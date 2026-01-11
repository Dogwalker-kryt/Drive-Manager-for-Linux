# Partition Management System Documentation  
Includes: **PartitionsUtils** (low‑level operations) and **listpartisions()** (interactive UI)

This module provides a complete interface for inspecting and modifying drive partitions.  
It includes:

- listing partitions  
- resizing partitions  
- moving partitions  
- changing partition types  
- safety confirmations  
- error handling and logging  

It is built around two components:

1. **PartitionsUtils** — low‑level wrappers around `parted` and `sfdisk`  
2. **listpartisions()** — interactive TUI for selecting and modifying partitions  

------------------------------------------------------------
# PartitionsUtils Class

This class provides static helper functions that wrap partition‑modifying system commands.  
Each function returns **true on success** and **false on failure**.

------------------------------------------------------------
## 1. `resizePartition()`

    ```cpp
    static bool resizePartition(const std::string& device, uint64_t newSizeMB)
    ```

Resizes **partition 1** of the given device to a new size in megabytes.

### Behavior:

- Builds a `parted --script` command:

        parted --script <device> resizepart 1 <newSizeMB>MB

- Executes it using `runTerminal()`
- Returns **false** if:
  - the command output contains `"error"`
  - an exception occurs

This is a destructive operation and should be used with caution.

------------------------------------------------------------
## 2. `movePartition()`

    ```cpp
    static bool movePartition(const std::string& device, int partNum, uint64_t startSectorMB)
    ```

Moves a partition to a new starting position.

### Behavior:

- Builds a command:

        parted --script <device> move <partNum> <startSectorMB>MB

- Executes via `runTerminal()`
- Returns **false** on errors or exceptions

Moving partitions can cause data loss if not done carefully.

------------------------------------------------------------
## 3. `changePartitionType()`

    ```cpp
    static bool changePartitionType(const std::string& device, int partNum, const std::string& newType)
    ```

Changes the partition type code (e.g., `83`, `7`, `b`, `82`).

### Behavior:

1. Creates a backup of the partition table:

        sfdisk -d <device> > <device>_backup.sf

2. Changes the partition type:

        echo 'type=<newType>' | sfdisk --part-type <device> <partNum>

3. Returns **false** if `"error"` appears in output or an exception occurs.

This operation can make data unreadable if the wrong type is chosen.

------------------------------------------------------------
# listpartisions() Function

This function provides a **full interactive UI** for partition management.

------------------------------------------------------------
## Step 1 — Drive Selection

The user selects a drive via TUI:

    ```cpp
    std::string drive_name = listDrives(true);
    ```

------------------------------------------------------------
## Step 2 — List Partitions

The function runs:

    lsblk --ascii -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -n -p <drive>

It prints a formatted table:

- Name  
- Size  
- Type  
- Mountpoint  
- FSType  

Only entries containing `"part"` are considered partitions.

Each partition name is stored in:

    std::vector<std::string> partitions;

If no partitions exist, the function exits.

------------------------------------------------------------
## Step 3 — Partition Management Menu

The user is shown:

    1. Resize partition  
    2. Move partition  
    3. Change partition type  
    4. Return to main menu  

The user selects an action.

------------------------------------------------------------
# Case 1 — Resize Partition

### Steps:

1. Ask for partition number  
2. Ask for new size in MB  
3. Confirm with warning  
4. Call:

        PartitionsUtils::resizePartition(partitions[partNum-1], newSize)

5. Print success or failure  
6. Log errors if needed  

------------------------------------------------------------
# Case 2 — Move Partition

### Steps:

1. Ask for partition number  
2. Ask for new start position in MB  
3. Confirm with warning  
4. Call:

        PartitionsUtils::movePartition(partitions[partNum-1], partNum, startPos)

5. Print success or failure  
6. Log errors  

------------------------------------------------------------
# Case 3 — Change Partition Type

### Steps:

1. Ask for partition number  
2. Show available types:

- Linux (83)  
- NTFS (7)  
- FAT32 (b)  
- Linux swap (82)  

3. Ask for type number  
4. Confirm with warning  
5. Call:

        PartitionsUtils::changePartitionType(drive_name, partNum, newType)

6. Print success or failure  
7. Log errors  

------------------------------------------------------------
# Case 4 — Return to Main Menu

Simply exits the function.

------------------------------------------------------------
# Summary

This module provides a complete partition management system:

- **PartitionsUtils** handles low‑level operations:
  - resizing  
  - moving  
  - changing type  

- **listpartisions()** provides:
  - drive selection  
  - partition listing  
  - interactive menu  
  - safety confirmations  
  - error logging  

It integrates tightly with DriveMgr’s TUI and ensures destructive operations are always deliberate and traceable.

------------------------------------------------------------
