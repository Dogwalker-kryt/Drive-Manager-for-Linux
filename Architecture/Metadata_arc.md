# MetadataReader Class Documentation  
Includes: **DriveMetadata struct**, **getMetadata()**, **displayMetadata()**, **mainReader()**

---

## `DriveMetadata` Struct

```cpp
struct DriveMetadata {
    std::string name;
    std::string size;
    std::string model;
    std::string serial;
    std::string type;
    std::string mountpoint;
    std::string vendor;
    std::string fstype;
    std::string uuid;
};
```

### Overview
A simple container for all metadata fields extracted from `lsblk` JSON output.  
Used internally by `MetadataReader` to pass structured drive information.

---

## `getMetadata()`

```cpp
static DriveMetadata getMetadata(const std::string& drive) {
    DriveMetadata metadata;
    std::string cmd = "lsblk -J -o NAME,SIZE,MODEL,SERIAL,TYPE,MOUNTPOINT,VENDOR,FSTYPE,UUID -p " + drive;
    std::string json = runTerminalV3(cmd);
    size_t deviceStart = json.find("{", json.find("["));
    size_t childrenPos = json.find("\"children\"", deviceStart);
    std::string deviceBlock = json.substr(deviceStart, childrenPos - deviceStart);

    auto extractValue = [&](const std::string& key, const std::string& from) -> std::string {
        std::regex pattern("\"" + key + "\"\\s*:\\s*(null|\"(.*?)\")");
        std::smatch match;
        if (std::regex_search(from, match, pattern)) {
            if (match[1] == "null")
                return "";
            else
                return match[2].str();
        }
        return "";
    };

    metadata.name       = extractValue("name", deviceBlock);
    metadata.size       = extractValue("size", deviceBlock);
    metadata.model      = extractValue("model", deviceBlock);
    metadata.serial     = extractValue("serial", deviceBlock);
    metadata.type       = extractValue("type", deviceBlock);
    metadata.mountpoint = extractValue("mountpoint", deviceBlock);
    metadata.vendor     = extractValue("vendor", deviceBlock);
    metadata.fstype     = extractValue("fstype", deviceBlock);
    metadata.uuid       = extractValue("uuid", deviceBlock);
    return metadata;
}
```

### Overview
Extracts drive metadata by parsing JSON output from:

```
lsblk -J -o NAME,SIZE,MODEL,SERIAL,TYPE,MOUNTPOINT,VENDOR,FSTYPE,UUID -p <drive>
```

### Behavior
- Runs `lsblk` in JSON mode.
- Locates the JSON object for the drive.
- Uses regex to extract fields safely.
- Handles `"null"` values by returning empty strings.
- Returns a fully populated `DriveMetadata` struct.

### Notes
- Only top‑level drive metadata is extracted (not partition metadata).
- Uses `runTerminalV3()` for improved command execution handling.

---

## `displayMetadata()`

```cpp
static void displayMetadata(const DriveMetadata& metadata) {
    std::cout << "\n-------- Drive Metadata --------\n";
    std::cout << "| Name:       " << metadata.name << "\n";
    std::cout << "| Size:       " << metadata.size << "\n";
    std::cout << "| Model:      " << (metadata.model.empty() ? "N/A" : metadata.model) << "\n";
    std::cout << "| Serial:     " << (metadata.serial.empty() ? "N/A" : metadata.serial) << "\n";
    std::cout << "| Type:       " << metadata.type << "\n";
    std::cout << "| Mountpoint: " << (metadata.mountpoint.empty() ? "Not mounted" : metadata.mountpoint) << "\n";
    std::cout << "| Vendor:     " << (metadata.vendor.empty() ? "N/A" : metadata.vendor) << "\n";
    std::cout << "| Filesystem: " << (metadata.fstype.empty() ? "N/A" : metadata.fstype) << "\n";
    std::cout << "| UUID:       " << (metadata.uuid.empty() ? "N/A" : metadata.uuid) << "\n";

    if (metadata.type == "disk") {
        std::cout << "\n┌-─-─-─- SMART Data -─-─-─-─\n";
        std::string smartCmd = "sudo smartctl -i " + metadata.name;
        std::string smartOutput = runTerminal(smartCmd);
        if (!smartOutput.empty()) {
            std::cout << smartOutput;
        } else {
            std::cout << "[ERROR] SMART data not available/intalled\n";
        }
    }
    std::cout << "└─  - -─ --- ─ - -─-  - ──- ──- ───────────────────\n";
}
```

### Overview
Prints all metadata fields in a clean, readable format.

### Behavior
- Displays:
  - name  
  - size  
  - model  
  - serial  
  - type  
  - mountpoint  
  - vendor  
  - filesystem  
  - UUID  
- If the device is a **disk**, attempts to show SMART info using:
  ```
  sudo smartctl -i <device>
  ```
- Handles missing SMART support gracefully.

---

## `mainReader()`

```cpp
static void mainReader() {
    try{
        std::string driveName = listDrives(true);

        try {
            DriveMetadata metadata = getMetadata(driveName);
            displayMetadata(metadata);
            Logger::log("[INFO] Successfully read metadata for drive: " + driveName);
        } catch (const std::exception& e) {
            std::cout << RED << "[ERROR] Failed to read drive metadata: " << e.what() << RESET << "\n";
            Logger::log("[ERROR] Failed to read drive metadata: " + std::string(e.what()));
            return;
        }
    } catch (const std::exception& e) {
        std::cerr << RED << "[ERROR] Failed to initialize metadata reading" << RESET << "\n";
        return;
    }
}
```

### Overview
Entry point for the metadata reader.

### Behavior
1. Lets the user select a drive via `listDrives(true)`.
2. Calls `getMetadata()` to extract metadata.
3. Calls `displayMetadata()` to print it.
4. Logs success or failure.
5. Handles exceptions at both selection and parsing stages.

---

## Summary

The **MetadataReader** class provides:

- JSON‑based metadata extraction via `lsblk`
- regex‑based parsing for robustness
- SMART info integration
- clean formatted output
- full error handling and logging

It is a reliable subsystem for inspecting drive properties in Drive Manager.

