# Clone Class Documentation  
Includes: **CloneDrive()** and **mainClone()**

---

## `CloneDrive()`

```cpp
static void CloneDrive(const std::string &source, const std::string &target) {
    std::cout << "\n[CloneDrive] Do you want to clone data from " << source << " to " << target << "? This will overwrite all data on the target drive(n) (y/n): ";
    char confirmation = 'n';
    std::cin >> confirmation;

    if (confirmation != 'y' && confirmation != 'Y') {
        std::cout << "[Info] Operation cancelled\n";
        Logger::log("[INFO] Operation cancelled -> Clone::CloneDrive()");
        return;

    } else if (confirmation == 'y' || confirmation == 'Y') {
        try {
            std::string clone_cmd = "sudo dd if=" + source + " of=" + target + " bs=4M status=progress && sync";
            std::string clone_cmd_output = Terminalexec::execTerminalv2(clone_cmd.c_str());
            if (clone_cmd_output.find("error") != std::string::npos) {
                Logger::log("[ERROR] Failed to clone drive from " + source + " to " + target + " -> Clone::CloneDrive()");
                throw std::runtime_error(clone_cmd_output);
            }
            std::cout << "[Success] Drive cloned from " << source << " to " << target << "\n";
            Logger::log("[INFO] Drive cloned successfully from " + source + " to " + target + " -> Clone::CloneDrive()");
        
        } catch (const std::exception& e) {
            std::cout << RED << "[ERROR] Failed to clone drive: " << e.what() << RESET << "\n";
            Logger::log(std::string("[ERROR] Failed to clone drive from ") + source + " to " + target + " -> Clone::CloneDrive(): " + e.what());
            return;
        }
    }
}
```

### Overview
Clones one entire drive to another using the `dd` utility.  
This is a **destructive operation** that overwrites the target drive completely.

### Behavior
- Prompts the user for confirmation.
- If confirmed:
  - Executes:  
    `sudo dd if=<source> of=<target> bs=4M status=progress && sync`
  - Checks for `"error"` in output.
  - Logs success or failure.
- If not confirmed:
  - Cancels operation and logs it.

### Safety
- Requires explicit user confirmation.
- Uses large block size (4MB) for faster cloning.
- Sync ensures all data is flushed to disk.

---

## `mainClone()`

```cpp
static void mainClone() {
    try {
        std::string sourceDrive = listDrives(true);

        std::cout << "\nEnter the target drive name to clone the data on to:\n";
        std::string targetDrive;
        std::cin >> targetDrive;
        checkDriveName(targetDrive);

        if (sourceDrive == targetDrive) {
            Logger::log("[ERROR] Source and target drives are the same -> Clone::mainClone()");
            throw std::runtime_error("Source and target drives cannot be the same!");
            return;
        } else {
            CloneDrive(sourceDrive, targetDrive);
            return;
        }
    } catch (std::exception& e) {
        std::cerr << RED << "[ERROR] " << e.what() << RESET << "\n";
        Logger::log("[ERROR] " + std::string(e.what()));
        return;
    }
}
```

### Overview
Provides the user interface for cloning drives.

### Behavior
1. User selects the **source drive** via `listDrives(true)`.
2. User enters the **target drive** manually.
3. `checkDriveName()` validates the target.
4. Prevents cloning a drive onto itself.
5. Calls `CloneDrive()` to perform the actual cloning.
6. Catches and logs any exceptions.

### Safety
- Ensures source and target are not identical.
- Validates target drive exists.
- Wraps entire operation in a try/catch block.

---

## Summary

The **Clone** class provides a safe wrapper around the `dd` cloning process:

- **CloneDrive()**  
  Performs the actual cloning with confirmation and error handling.

- **mainClone()**  
  Handles user input, validation, and dispatching.

This system ensures that cloning is deliberate, logged, and protected against common user mistakes.

