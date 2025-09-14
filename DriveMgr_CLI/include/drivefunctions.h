
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
        static void log(const std::string& operation) {
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&currentTime));
        std::string logMsg = "[" + std::string(timeStr) + "] executed " + operation;

        const char* sudo_user = std::getenv("SUDO_USER");
        const char* user_env = std::getenv("USER");
        const char* username = sudo_user ? sudo_user : user_env;
        if (!username) {
            std::cerr << "[Logger Error] Could not determine username.\n";
            return;
        }
        struct passwd* pw = getpwnam(username);
        if (!pw) {
            std::cerr << "[Logger Error] Failed to get home directory for user: " << username << "\n";
            return;
        }
        std::string homeDir = pw->pw_dir;
        std::string logDir = homeDir + "/.var/app/DriveMgr/";
        struct stat st;
        if (stat(logDir.c_str(), &st) != 0) {
            if (mkdir(logDir.c_str(), 0755) != 0 && errno != EEXIST) {
                std::cerr << "[Logger Error] Failed to create log directory: " << logDir
                          << " Reason: " << strerror(errno) << "\n";
                return;
            }
        }

        std::string logPath = logDir + "log.dat";
        std::ofstream logFile(logPath, std::ios::app);
        if (logFile) {
            logFile << logMsg << std::endl;
        } else {
            std::cerr << "[Logger Error] Unable to open log file: " << logPath
                      << " Reason: " << strerror(errno) << "\n";
        }
    }
};
};

// Command executer
class Terminalexec {
public:
    static std::string execTerminal(const char* cmd) {
        std::array<char, 128> buffer;
        std::string result;
        auto deleter = [](FILE* f) { if (f) pclose(f); };
        std::unique_ptr<FILE, decltype(deleter)> pipe(popen(cmd, "r"), deleter);
        if (!pipe) throw std::runtime_error("popen() failed!");
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }
    //v2
    static std::string execTerminalv2(const std::string &command) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe) return "";
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }
    //v3
    static std::string execTerminalv3(const std::string& cmd) {
        std::array<char, 512> buffer;
        std::string result;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            throw std::runtime_error("popen() failed");
        }
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        int returnCode = pclose(pipe);
        if (returnCode != 0) {
            return ""; 
        }
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
        return result;
    }
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
