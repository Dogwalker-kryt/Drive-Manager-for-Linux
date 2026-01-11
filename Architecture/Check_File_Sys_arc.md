# checkFilesystem() Function Documentation

```cpp
std::string checkFilesystem(const std::string& device, const std::string& fstype) {
    if (fstype.empty()) return "Unknown filesystem";
    std::string result;

    try {
        std::string cmd;
        if (fstype == "ext4" || fstype == "ext3" || fstype == "ext2") {
            cmd = "e2fsck -n " + device + " 2>&1";
        } else if (fstype == "ntfs") {
            cmd = "ntfsfix --no-action " + device + " 2>&1";
        } else if (fstype == "vfat" || fstype == "fat32") {
            cmd = "dosfsck -n " + device + " 2>&1";
        }
        if (!cmd.empty()) {
            result = runTerminal(cmd);
        }
    } catch (const std::exception& e) {
        return "Check failed: " + std::string(e.what());
    }

    if (result.find("clean") != std::string::npos || result.find("no errors") != std::string::npos) {
        return "Clean";
    } else if (!result.empty()) {
        return "Issues found";
    }
    return "Unknown state";
}
```

## Overview

The `checkFilesystem()` function performs a **read‑only filesystem health check** on a block device using the correct tool for its filesystem type.  
It returns a short, human‑friendly status string.

## Behavior Summary

- Empty `fstype` → `"Unknown filesystem"`
- ext2/ext3/ext4 → uses `e2fsck -n`
- NTFS → uses `ntfsfix --no-action`
- FAT32/VFAT → uses `dosfsck -n`
- Output contains `"clean"` or `"no errors"` → `"Clean"`
- Output non‑empty but not clean → `"Issues found"`
- No output → `"Unknown state"`
- Exception → `"Check failed: <error>"`

## Return Values

- **Clean** — filesystem reports no errors  
- **Issues found** — warnings or errors detected  
- **Unknown filesystem** — cannot determine check tool  
- **Unknown state** — no output to interpret  
- **Check failed** — command execution error  
