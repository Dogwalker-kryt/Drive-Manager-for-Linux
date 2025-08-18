
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


class Logger {
public:
    static void log(const std::string& operation) {
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&currentTime));
        
        std::string logMsg = std::string("[") + timeStr + "] executed " + operation;
        
        std::cout << logMsg << std::endl;

        std::string logPath = std::string(getenv("HOME")) + "/.var/log/DriveMgr/operations.log";
        
        std::ofstream logFile(logPath, std::ios::app);
        if (logFile) {
            logFile << logMsg << std::endl;
        }
    }
};

// execTerminal functions
std::string execTerminal2ZeroDrive(const std::string &command) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string execTerminal(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    auto deleter = [](FILE* f) { if (f) pclose(f); };
    std::unique_ptr<FILE, decltype(deleter)> pipe(popen(cmd, "r"), deleter);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
} // end of execTerminal

// Partiotion
bool resizePartition(const std::string& device, uint64_t newSizeMB) {
    try {
        std::string cmd = "parted --script " + device + " resizepart 1 " + 
                         std::to_string(newSizeMB) + "MB";
        std::string output = execTerminal(cmd.c_str());
        return output.find("error") == std::string::npos;
    } catch (const std::exception&) {
        return false;
    }
}

bool movePartition(const std::string& device, int partNum, uint64_t startSectorMB) {
    try {
        std::string cmd = "parted --script " + device + " move " + 
                         std::to_string(partNum) + " " + 
                         std::to_string(startSectorMB) + "MB";
        std::string output = execTerminal(cmd.c_str());
        return output.find("error") == std::string::npos;
    } catch (const std::exception&) {
        return false;
    }
}

bool changePartitionType(const std::string& device, int partNum, const std::string& newType) {
    try {
        std::string backupCmd = "sfdisk -d " + device + " > " + device + "_backup.sf";
        execTerminal(backupCmd.c_str());

        std::string cmd = "echo 'type=" + newType + "' | sfdisk --part-type " + 
                         device + " " + std::to_string(partNum);
        std::string output = execTerminal(cmd.c_str());
        return output.find("error") == std::string::npos;
    } catch (const std::exception&) {
        return false;
    }
}
// partiotion end

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

#endif 
