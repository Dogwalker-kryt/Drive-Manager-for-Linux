
#ifndef DRIVEFUNCTIONS_H
#define DRIVEFUNCTIONS_H

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <fstream>
#include <ostream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <array>
#include <limits>
#include <iomanip>
#include <filesystem>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
/*
class Logger {
public:
    static void log(const std::string& operation) {
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&currentTime));

        std::string logMsg = std::string("[") + timeStr + "] executed " + operation;
        
        std::string logDir = std::string(getenv("HOME")) + "/" + ".var/app/DriveMgr";
        std::string logPath = logDir + "/log.dat";
        std::ofstream logFile(logPath, std::ios::app);
        if (logFile) {
            logFile << logMsg << std::endl;
        } else {
            std::cerr << "[Error] Unable to open log file: " << logPath << " Reason: " << strerror(errno) << std::endl;
        }
    }
};
*/
class Logger {
public:
    static void log(const std::string& operation);
};

// Command executer
class Terminalexec {
public:
    static std::string execTerminal(const char* cmd);
    //v2
    static std::string execTerminalv2(const std::string &command);
    //v3
    static std::string execTerminalv3(const std::string& cmd);
};

struct DriveInfo {
    std::string device;
    std::string size;
    std::string type;
    std::string mountpoint;
    std::string label;
    std::string fstype;
    bool isEncrypted;
    bool hasErrors;

};

// encryption
const std::string KEY_STORAGE_PATH = std::string(getenv("HOME")) + "/.var/app/DriveMgr/key.bin";

struct EncryptionInfo {
    std::string driveName;
    unsigned char key[32];  // 256-bit key
    unsigned char iv[16];   // 128-bit IV for CBC mode
};



#endif 
