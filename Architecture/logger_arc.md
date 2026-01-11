# Logger Class Documentation

## Overview

The `Logger` class provides a centralized logging system for DriveMgr.  
It records timestamped log entries into:

    ~/.local/share/DriveMgr/data/log.dat

Each entry includes:

- a formatted timestamp  
- the executed operation text  
- automatic directory creation if missing  
- error handling for missing users or file failures  

This ensures DriveMgr maintains a persistent and readable activity history.

------------------------------------------------------------

## `log()` — Write a Timestamped Log Entry

The logger exposes a single static function:

    ```cpp
    static void log(const std::string& operation)
    ```

It appends a formatted log entry to the log file.

------------------------------------------------------------

## Step‑by‑Step Breakdown

### 1. Generate Timestamp

    ```cpp
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%d-%m-%Y %H:%M", std::localtime(&currentTime));
    std::string logMsg = "[" + std::string(timeStr) + "] executed " + operation;
    ```

The timestamp format is:

    DD-MM-YYYY HH:MM

------------------------------------------------------------

### 2. Determine the Correct User

The logger resolves the real user (even under sudo):

    ```cpp
    const char* sudo_user = std::getenv("SUDO_USER");
    const char* user_env = std::getenv("USER");
    const char* username = sudo_user ? sudo_user : user_env;
    ```

If no username is found, logging stops with an error.

------------------------------------------------------------

### 3. Resolve Home Directory

    ```cpp
    struct passwd* pw = getpwnam(username);
    if (!pw) {
        std::cerr << "[Logger Error] Failed to get home directory for user\n";
        return;
    }
    ```

------------------------------------------------------------

### 4. Ensure Log Directory Exists

The log directory is:

    ~/.local/share/DriveMgr/data/

The logger checks and creates it if needed:

    ```cpp
    struct stat st;
    if (stat(logDir.c_str(), &st) != 0) {
        if (mkdir(logDir.c_str(), 0755) != 0 && errno != EEXIST) {
            std::cerr << "[Logger Error] Failed to create log directory\n";
            return;
        }
    }
    ```

------------------------------------------------------------

### 5. Append Log Entry to File

The log file path:

    ```cpp
    std::string logPath = logDir + "log.dat";
    ```

Open in append mode:

    ```cpp
    std::ofstream logFile(logPath, std::ios::app);
    ```

Write the entry:

    ```cpp
    if (logFile) {
        logFile << logMsg << std::endl;
    } else {
        std::cerr << "[Logger Error] Unable to open log file\n";
    }
    ```

------------------------------------------------------------

## Summary

The `Logger` class:

- timestamps every log entry  
- resolves the correct user (sudo‑aware)  
- ensures the log directory exists  
- appends logs safely  
- prints detailed error messages on failure  

It is a core component for tracking DriveMgr’s internal operations.

------------------------------------------------------------
