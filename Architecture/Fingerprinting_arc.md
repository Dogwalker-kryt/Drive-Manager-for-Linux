# DriveFingerprinting Class Documentation

## Overview

The `DriveFingerprinting` class is responsible for generating a **unique fingerprint** for a selected drive.  
It does this by:

- retrieving hardware metadata (name, size, model, serial, UUID)  
- combining that metadata into a single string  
- hashing it using **SHA‑256**  
- outputting the resulting fingerprint  

This allows DriveMgr to uniquely identify drives even if their mount points or device paths change.

------------------------------------------------------------

## Internal Structure: `DriveMetadata`

The class defines a private struct to store all relevant metadata fields:

    ```cpp
    struct DriveMetadata {
        std::string name;
        std::string size;
        std::string model;
        std::string serial;
        std::string uuid;
    };
    ```

These fields are extracted from the system using `lsblk` in JSON mode.

------------------------------------------------------------

## `getMetadata()` — Retrieving Drive Information

This private static function gathers metadata for a given drive path.

    ```cpp
    static DriveMetadata getMetadata(const std::string& drive)
    ```

### Steps performed:

1. **Builds an lsblk command** to retrieve drive information in JSON format:

        ```cpp
        std::string cmd =
            "lsblk -J -o NAME,SIZE,MODEL,SERIAL,TYPE,MOUNTPOINT,VENDOR,FSTYPE,UUID -p " + drive;
        ```

2. **Runs the command** and captures the JSON output:

        ```cpp
        std::string json = runTerminalV3(cmd);
        ```

3. **Extracts the device block** from the JSON by locating the first `{` after the array start and stopping before `"children"`:

        ```cpp
        size_t deviceStart = json.find("{", json.find("["));
        size_t childrenPos = json.find("\"children\"", deviceStart);
        std::string deviceBlock = json.substr(deviceStart, childrenPos - deviceStart);
        ```

4. **Defines a lambda** to extract individual JSON fields using regex:

        ```cpp
        auto extractValue = [&](const std::string& key, const std::string& from) -> std::string {
            std::regex pattern("\"" + key + "\"\\s*:\\s*(null|\"(.*?)\")");
            ...
        };
        ```

5. **Extracts all metadata fields**:

        ```cpp
        metadata.name   = extractValue("name", deviceBlock);
        metadata.size   = extractValue("size", deviceBlock);
        metadata.model  = extractValue("model", deviceBlock);
        metadata.serial = extractValue("serial", deviceBlock);
        metadata.uuid   = extractValue("uuid", deviceBlock);
        ```

6. **Returns the populated struct**.

------------------------------------------------------------

## `fingerprinting()` — SHA‑256 Hash Generator

This private static function generates a SHA‑256 hash from a given input string.

    ```cpp
    static std::string fingerprinting(const std::string& input)
    ```

### Steps performed:

1. **Computes SHA‑256**:

        ```cpp
        SHA256((unsigned char*)input.c_str(), input.size(), hash);
        ```

2. **Converts the hash bytes to a hex string**:

        ```cpp
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            char hex[3];
            snprintf(hex, sizeof(hex), "%02x", hash[i]);
            fingerprint += hex;
        }
        ```

3. **Logs an error** if hashing fails:

        ```cpp
        if (fingerprint.empty()) {
            Logger::log("[ERROR] Failed to generate fingerprint...");
            return "";
        }
        ```

4. **Returns the final fingerprint**.

------------------------------------------------------------

## `fingerprinting_main()` — User‑Facing Fingerprint Workflow

This public static function is the entry point for the fingerprinting feature.

    ```cpp
    static void fingerprinting_main()
    ```

### Steps performed:

1. **Prompts the user to select a drive**:

        ```cpp
        std::string drive_name_fingerprinting = listDrives(true);
        ```

2. **Retrieves metadata** for the selected drive:

        ```cpp
        DriveMetadata metadata = getMetadata(drive_name_fingerprinting);
        ```

3. **Logs the retrieval**:

        ```cpp
        Logger::log("[INFO] Retrieved metadata for drive: " + drive_name_fingerprinting);
        ```

4. **Combines metadata into a single string** separated by `|`:

        ```cpp
        std::string combined_metadatat =
            metadata.name + "|" +
            metadata.size + "|" +
            metadata.model + "|" +
            metadata.serial + "|" +
            metadata.uuid;
        ```

5. **Generates the fingerprint**:

        ```cpp
        std::string fingerprint = fingerprinting(combined_metadatat);
        ```

6. **Logs the fingerprint generation**:

        ```cpp
        Logger::log("[INFO] Generated fingerprint for drive: " + drive_name_fingerprinting);
        ```

7. **Displays the fingerprint to the user**:

        ```cpp
        std::cout << "A custom fingerprint was made for your drive\n";
        std::cout << BOLD << "Fingerprint:\n" << RESET;
        std::cout << fingerprint << "\n";
        ```

------------------------------------------------------------

## Summary

The `DriveFingerprinting` class:

- extracts hardware metadata using `lsblk`  
- parses JSON manually using regex  
- combines metadata into a unique identifier string  
- hashes it using SHA‑256  
- outputs a reproducible fingerprint for the drive  

This fingerprint can be used for:

- drive verification  
- forensic analysis  
- tracking hardware changes  
- identifying drives across reboots or system changes  

------------------------------------------------------------
