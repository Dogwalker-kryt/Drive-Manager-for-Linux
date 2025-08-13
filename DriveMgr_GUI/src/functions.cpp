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
#include <fstream>
#include <filesystem>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <cstring>
#include <functional>
#include "functions.h"
#include "drivefunctions.h"
#include "encryption.h"

namespace {

std::string execTerminal(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

} // anonymous namespace

namespace gui {

std::vector<DriveInfo> listDrives() {
    std::vector<DriveInfo> result;
    std::string lsblk = execTerminal("lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,LABEL,FSTYPE -d -n -p");
    std::istringstream iss(lsblk);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (line.find("disk") != std::string::npos) {
            std::istringstream lss(line);
            DriveInfo info;
            lss >> info.device >> info.size >> info.type;
            
            // Read mountpoint, label, and fstype
            std::string rest;
            std::getline(lss, rest);
            std::istringstream rss(rest);
            std::string token;
            int field = 0;
            while (rss >> token) {
                switch (field++) {
                    case 0: info.mountpoint = token; break;
                    case 1: info.label = token; break;
                    case 2: info.fstype = token; break;
                }
            }
            
            // Check if drive is encrypted using cryptsetup
            std::string cryptsetup = execTerminal(("cryptsetup status " + info.device + " 2>/dev/null").c_str());
            info.isEncrypted = (cryptsetup.find("active") != std::string::npos);
            
            result.push_back(info);
        }
    }
    return result;
}

bool formatDrive(const DriveInfo& drive, const char* fstype, const char* label, 
                std::function<void(int)> progress_callback) {
    if (progress_callback) progress_callback(0);
    
    try {
        std::string cmd = "mkfs." + std::string(fstype) + " ";
        if (label && *label) {
            cmd += "-L " + std::string(label) + " ";
        }
        cmd += drive.device;
        
        std::string output = execTerminal(cmd.c_str());
        
        if (progress_callback) progress_callback(100);
        
        return output.find("error") == std::string::npos &&
               output.find("failed") == std::string::npos;
    } catch (const std::exception&) {
        return false;
    }
}

bool encryptDrive(const char* device, const char* password, 
                 std::function<void(double)> progress_callback) {
    return EncryptionManager::getInstance().encryptDrive(device, password, progress_callback);
}

bool decryptDrive(const char* device, const char* password,
                 std::function<void(double)> progress_callback) {
    return EncryptionManager::getInstance().decryptDrive(device, password, progress_callback);
}

bool zeroDrive(const char* device, std::function<void(double)> progress_callback) {
    if (progress_callback) progress_callback(0.0);
    
    try {
        // First pass: write zeros
        std::string cmd = "dd if=/dev/zero of=" + std::string(device) + " bs=1M status=progress";
        std::string output = execTerminal(cmd.c_str());
        
        if (output.find("error") != std::string::npos ||
            output.find("failed") != std::string::npos) {
            return false;
        }
        
        if (progress_callback) progress_callback(50.0);
        
        // Second pass: write random data
        cmd = "dd if=/dev/urandom of=" + std::string(device) + " bs=1M status=progress";
        output = execTerminal(cmd.c_str());
        
        if (progress_callback) progress_callback(100.0);
        
        return output.find("error") == std::string::npos &&
               output.find("failed") == std::string::npos;
    } catch (const std::exception&) {
        return false;
    }
}

SpaceInfo analyzeSpace(const char* mountpoint) {
    SpaceInfo result;
    
    try {
        // Get total space information using df
        std::string cmd = "df -B1 '" + std::string(mountpoint) + "'";
        std::string output = execTerminal(cmd.c_str());
        std::istringstream iss(output);
        std::string line;
        std::getline(iss, line); // Skip header
        if (std::getline(iss, line)) {
            std::istringstream lss(line);
            std::string device;
            lss >> device >> result.totalSpace >> result.usedSpace >> result.freeSpace;
        }
        
        // Get top directories by size using du
        cmd = "du -b --max-depth=1 '" + std::string(mountpoint) + "' | sort -nr | head -n 10";
        output = execTerminal(cmd.c_str());
        iss = std::istringstream(output);
        while (std::getline(iss, line)) {
            std::istringstream lss(line);
            uint64_t size;
            std::string path;
            if (lss >> size) {
                std::getline(lss, path);
                if (!path.empty() && path[0] == ' ') path = path.substr(1);
                result.topDirectories.push_back({path, size});
            }
        }
    } catch (const std::exception&) {
        // Handle errors
    }
    
    return result;
}

DriveHealth checkDriveHealth(const char* device) {
    DriveHealth result;
    result.isHealthy = false;
    
    try {
        std::string cmd = "smartctl -a " + std::string(device);
        std::string output = execTerminal(cmd.c_str());
        std::istringstream iss(output);
        std::string line;
        
        // Parse SMART information
        while (std::getline(iss, line)) {
            if (line.find("SMART overall-health self-assessment test result:") != std::string::npos) {
                size_t pos = line.find_last_of(':');
                if (pos != std::string::npos) {
                    result.smartStatus = line.substr(pos + 2);
                    result.isHealthy = (result.smartStatus.find("PASSED") != std::string::npos);
                }
            }
            else if (line.find("Temperature_Celsius") != std::string::npos) {
                std::istringstream lss(line);
                std::string token;
                while (lss >> token) {
                    if (std::isdigit(token[0])) {
                        result.temperature = std::stoi(token);
                        break;
                    }
                }
            }
            else if (line.find("Power_On_Hours") != std::string::npos) {
                std::istringstream lss(line);
                std::string token;
                while (lss >> token) {
                    if (std::isdigit(token[0])) {
                        result.powerOnHours = std::stoi(token);
                        break;
                    }
                }
            }
            else if (line.find("Reallocated_Sector_Ct") != std::string::npos) {
                std::istringstream lss(line);
                std::string token;
                while (lss >> token) {
                    if (std::isdigit(token[0])) {
                        result.reallocatedSectors = std::stoi(token);
                        break;
                    }
                }
            }
            else {
                // Look for other SMART attributes
                std::istringstream lss(line);
                int id;
                std::string name, value;
                if (lss >> id && std::getline(lss, name) && std::getline(lss, value)) {
                    result.smartAttributes.push_back({name, value});
                }
            }
        }
    } catch (const std::exception&) {
        // Handle errors
    }
    
    return result;
}

} // namespace gui

// C interface implementation
extern "C" {

void gui_listDrives(gui_DriveInfo** drives, int* count) {
    auto result = gui::listDrives();
    *count = result.size();
    if (*count > 0) {
        *drives = new gui_DriveInfo[*count];
        for (int i = 0; i < *count; i++) {
            strncpy((*drives)[i].device, result[i].device.c_str(), 255);
            strncpy((*drives)[i].size, result[i].size.c_str(), 31);
            strncpy((*drives)[i].type, result[i].type.c_str(), 31);
            strncpy((*drives)[i].mountpoint, result[i].mountpoint.c_str(), 255);
            strncpy((*drives)[i].label, result[i].label.c_str(), 255);
            strncpy((*drives)[i].fstype, result[i].fstype.c_str(), 31);
            (*drives)[i].isEncrypted = result[i].isEncrypted;
        }
    } else {
        *drives = nullptr;
    }
}

void gui_freeDrives(gui_DriveInfo* drives, int count) {
    delete[] drives;
}

bool gui_formatDrive(const char* device, const char* fstype, const char* label) {
    gui::DriveInfo drive;
    drive.device = device;
    return gui::formatDrive(drive, fstype, label, nullptr);
}

bool gui_encryptDrive(const char* device, const char* password) {
    return gui::encryptDrive(device, password, nullptr);
}

bool gui_decryptDrive(const char* device, const char* password) {
    return gui::decryptDrive(device, password, nullptr);
}

bool gui_zeroDrive(const char* device) {
    return gui::zeroDrive(device, nullptr);
}

gui_SpaceInfo* gui_analyzeSpace(const char* mountpoint) {
    auto result = gui::analyzeSpace(mountpoint);
    auto info = new gui_SpaceInfo;
    info->totalSpace = result.totalSpace;
    info->usedSpace = result.usedSpace;
    info->freeSpace = result.freeSpace;
    info->topDirectoryCount = std::min(10, static_cast<int>(result.topDirectories.size()));
    for (int i = 0; i < info->topDirectoryCount; i++) {
        strncpy(info->topDirectoryPaths[i], result.topDirectories[i].first.c_str(), 255);
        info->topDirectorySizes[i] = result.topDirectories[i].second;
    }
    return info;
}

void gui_freeSpaceInfo(gui_SpaceInfo* info) {
    delete info;
}

gui_DriveHealth* gui_checkDriveHealth(const char* device) {
    auto result = gui::checkDriveHealth(device);
    auto health = new gui_DriveHealth;
    health->isHealthy = result.isHealthy;
    health->temperature = result.temperature;
    health->powerOnHours = result.powerOnHours;
    health->reallocatedSectors = result.reallocatedSectors;
    strncpy(health->smartStatus, result.smartStatus.c_str(), 255);
    health->attributeCount = std::min(32, static_cast<int>(result.smartAttributes.size()));
    for (int i = 0; i < health->attributeCount; i++) {
        strncpy(health->attributeNames[i], result.smartAttributes[i].first.c_str(), 63);
        strncpy(health->attributeValues[i], result.smartAttributes[i].second.c_str(), 63);
    }
    return health;
}

void gui_freeDriveHealth(gui_DriveHealth* health) {
    delete health;
}

} // extern "C"
