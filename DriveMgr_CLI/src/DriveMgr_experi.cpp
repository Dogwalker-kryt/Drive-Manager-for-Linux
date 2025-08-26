/*
 * DriveMgr - Linux Drive Management Utility
 * Copyright (C) 2025 Dogwalker-kryt
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// ! Warning this version is teh experimentl version of the prorgam,
// This version has teh latest and newest functions, but my contain bugs and errors
// Curretn version of this code is in the Info() function below
// v0.8.88-18

#include <iostream>
#include <string>
#include <cstdlib>
#include "../include/drivefunctions.h"
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
#include <fstream>
#include "../include/encryption.h"
#include <regex>


// general side functions
// Logger
class Logger {
public:
    static void log(const std::string& operation) {
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&currentTime));

        std::string logMsg = std::string("[") + timeStr + "] executed " + operation;
        
        std::string logDir = std::string(getenv("HOME")) + "/.var/app/DriveMgr";
        std::string logPath = logDir + "/log.dat";
        std::ofstream logFile(logPath, std::ios::app);
        if (logFile) {
            logFile << logMsg << std::endl;
        } else {
            std::cerr << "[Error] Unable to open log file: " << logPath << " Reason: " << strerror(errno) << std::endl;
        }
    }
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



// cehskfielsytem
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
            result = Terminalexec::execTerminal(cmd.c_str());
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

// ListDrives
void listDrives(std::vector<std::string>& drives) {
    drives.clear();
    std::cout << "\nListing connected drives...\n";
    std::string lsblk = Terminalexec::execTerminal("lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -d -n -p");
    std::cout << "\nConnected Drives:\n";
    std::cout << std::left 
              << std::setw(3) << "#" 
              << std::setw(18) << "Device" 
              << std::setw(10) << "Size" 
              << std::setw(10) << "Type" 
              << std::setw(15) << "Mountpoint"
              << std::setw(10) << "FSType"
              << "Status" << std::endl;
    std::cout << std::string(90, '-') << "\n";
    std::istringstream iss(lsblk);
    std::string line;
    int idx = 0;
    while (std::getline(iss, line)) {
        if (line.find("disk") != std::string::npos) {
            std::istringstream lss(line);
            std::string device, size, type, mountpoint, fstype;
            lss >> device >> size >> type;
            
            std::string rest;
            std::getline(lss, rest);
            std::istringstream rss(rest);
            if (rss >> mountpoint >> fstype) {
                if (mountpoint == "-") mountpoint = "";
                if (fstype == "-") fstype = "";
            }
            std::string status = checkFilesystem(device, fstype);
            
            std::cout << std::left 
                      << std::setw(3) << idx 
                      << std::setw(18) << device 
                      << std::setw(10) << size 
                      << std::setw(10) << type 
                      << std::setw(15) << (mountpoint.empty() ? "" : mountpoint)
                      << std::setw(10) << (fstype.empty() ? "" : fstype)
                      << status << std::endl;
                      
            drives.push_back(device);
            idx++;
        }
    }
    if (drives.empty()) {
        std::cout << "No drives found!\n";
    }
}

// Partitions
class PartitionsUtils {
    public:
        // 1
        static bool resizePartition(const std::string& device, uint64_t newSizeMB) {
            try {
                std::string cmd = "parted --script " + device + " resizepart 1 " + 
                                 std::to_string(newSizeMB) + "MB";
                std::string output = Terminalexec::execTerminal(cmd.c_str());
                return output.find("error") == std::string::npos;
            } catch (const std::exception&) {
                return false;
            }
        }
        // 2
        static bool movePartition(const std::string& device, int partNum, uint64_t startSectorMB) {
            try {
                std::string cmd = "parted --script " + device + " move " + 
                                 std::to_string(partNum) + " " + 
                                 std::to_string(startSectorMB) + "MB";
                std::string output = Terminalexec::execTerminal(cmd.c_str());
                return output.find("error") == std::string::npos;
            } catch (const std::exception&) {
                return false;
            }
        }
        // 3
        static bool changePartitionType(const std::string& device, int partNum, const std::string& newType) {
            try {
                std::string backupCmd = "sfdisk -d " + device + " > " + device + "_backup.sf";
                Terminalexec::execTerminal(backupCmd.c_str());

                std::string cmd = "echo 'type=" + newType + "' | sfdisk --part-type " + 
                                 device + " " + std::to_string(partNum);
                std::string output = Terminalexec::execTerminal(cmd.c_str());
                return output.find("error") == std::string::npos;
            } catch (const std::exception&) {
                return false;
            }
        }
};

void listpartisions(std::vector<std::string>& drive) {
    std::cout << "\nListing partitions...\n";
    std::vector<std::string> drives;
    listDrives(drives);
    if (drives.empty()) {
        std::cout << "No drives found!\n";
        return;
    }
    std::cout << "\nEnter number of a drive you want to see the partisions of:\n";
    int listpartdrivenum;
    std::cin >> listpartdrivenum;
    if (listpartdrivenum < 0 || listpartdrivenum >= (int)drives.size()) {
        std::cout << "Invalid selection!\n";
        return;
    }

    std::string driveName = drives[listpartdrivenum];
    std::cout << "\nPartitions of drive " << driveName << ":\n";
    std::string cmd = "lsblk --ascii -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -n -p " + driveName;
    std::string output = Terminalexec::execTerminal(cmd.c_str());
    std::istringstream iss(output);
    std::string line;
    std::cout << std::left 
              << std::setw(25) << "Name" 
              << std::setw(10) << "Size" 
              << std::setw(10) << "Type" 
              << std::setw(15) << "Mountpoint"
              << "FSType" << "\n";
    std::cout << std::string(75, '-') << "\n";

    std::vector<std::string> partitions;
    while (std::getline(iss, line)) {
        if (line.find("part") != std::string::npos) {
            std::cout << line << "\n";
            std::istringstream lss(line);
            std::string partName;
            lss >> partName;
            partitions.push_back(partName);
        }
    }

    if (partitions.empty()) {
        std::cout << "No partitions found on this drive.\n";
        return;
    }

    std::cout << "\nPartition Management Options:\n";
    std::cout << "1. Resize partition\n";
    std::cout << "2. Move partition\n";
    std::cout << "3. Change partition type\n";
    std::cout << "4. Return to main menu\n";
    std::cout << "Enter your choice: ";

    int choice;
    std::cin >> choice;

    switch (choice) {
        case 1: {
            std::cout << "Enter partition number (1-" << partitions.size() << "): ";
            int partNum;
            std::cin >> partNum;
            if (partNum < 1 || partNum > (int)partitions.size()) {
                std::cout << "Invalid partition number!\n";
                break;
            }
            std::cout << "Enter new size in MB: ";
            uint64_t newSize;
            std::cin >> newSize;
            if (newSize <= 0) {
                std::cout << "Invalid size!\n";
                break;
            }
            std::cout << "[Warning] Resizing partitions can lead to data loss.\n";
            std::cout << "Are you sure? (y/n): ";
            char confirm;
            std::cin >> confirm;
            if (confirm == 'y' || confirm == 'Y') {
                if (PartitionsUtils::resizePartition(partitions[partNum-1], newSize)) {
                    std::cout << "Partition resized successfully!\n";
                } else {
                    std::cout << "Failed to resize partition!\n";
                }
            }
            break;
        }
        case 2: {
            std::cout << "Enter partition number (1-" << partitions.size() << "): ";
            int partNum;
            std::cin >> partNum;
            if (partNum < 1 || partNum > (int)partitions.size()) {
                std::cout << "Invalid partition number!\n";
                break;
            }
            std::cout << "Enter new start position in MB: ";
            uint64_t startPos;
            std::cin >> startPos;
            std::cout << "[Warning] Moving partitions can lead to data loss.\n";
            std::cout << "Are you sure? (y/n): ";
            char confirm;
            std::cin >> confirm;
            if (confirm == 'y' || confirm == 'Y') {
                if (PartitionsUtils::movePartition(partitions[partNum-1], partNum, startPos)) {
                    std::cout << "Partition moved successfully!\n";
                } else {
                    std::cout << "Failed to move partition!\n";
                    Logger::log("[ERROR] Failed to move partition");
                }
            }
            break;
        }
        case 3: {
            std::cout << "Enter partition number (1-" << partitions.size() << "): ";
            int partNum;
            std::cin >> partNum;
            if (partNum < 1 || partNum > (int)partitions.size()) {
                std::cout << "Invalid partition number!\n";
                break;
            }
            std::cout << "Available partition types:\n";
            std::cout << "1. Linux (83)\n";
            std::cout << "2. NTFS (7)\n";
            std::cout << "3. FAT32 (b)\n";
            std::cout << "4. Linux swap (82)\n";
            std::cout << "Enter type number: ";
            int typeNum;
            std::cin >> typeNum;
            std::string newType;
            switch (typeNum) {
                case 1: newType = "83"; break;
                case 2: newType = "7"; break;
                case 3: newType = "b"; break;
                case 4: newType = "82"; break;
                default:
                    std::cout << "Invalid type!\n";
                    break;
            }
            if (!newType.empty()) {
                std::cout << "[Warning] Changing partition type can make data inaccessible.\n";
                std::cout << "Are you sure? (y/n): ";
                char confirm;
                std::cin >> confirm;
                if (confirm == 'y' || confirm == 'Y') {
                    if (PartitionsUtils::changePartitionType(driveName, partNum, newType)) {
                        std::cout << "Partition type changed successfully!\n";
                    } else {
                        std::cout << "Failed to change partition type!\n";
                        Logger::log("[ERROR] Failed to change partition type");
                    }
                }
            }
            break;
        }
        case 4:
            return;
        default:
            std::cout << "Invalid option!\n";
    }
}

// analyzeDriskapce
void analyDiskSpace() {
    std::cout << "\nAnalyzing disk space...\n";
    std::vector<std::string> drives;
    listDrives(drives);
    if (drives.empty()) {
        std::cout << "No drives found!\n";
        return;
    }
    std::cout << "\nEnter the number of a drive you want to analyze:\n";
    int diskspacemenu1input;
    std::cin >> diskspacemenu1input;
    if (diskspacemenu1input < 0 || diskspacemenu1input >= (int)drives.size()) {
        std::cout << "Invalid selection!\n";
        return;
    }
    std::string driveName = drives[diskspacemenu1input];
    std::cout << "\n------ Disk Information ------\n";
    std::string cmd = "lsblk -b -o NAME,SIZE,TYPE,MOUNTPOINT -n -p " + driveName;
    std::string output = Terminalexec::execTerminal(cmd.c_str());
    std::istringstream iss(output);
    std::string line;
    bool found = false;
    std::string mountpoint;
    std::string size;
    while (std::getline(iss, line)) {
        std::istringstream lss(line);
        std::string name, type;
        lss >> name >> size >> type;
        std::getline(lss, mountpoint);
        if (!mountpoint.empty() && mountpoint[0] == ' ') mountpoint = mountpoint.substr(1);
        if (type == "disk") {
            found = true;
            std::cout << "Device:      " << name << "\n";
            try {
                unsigned long long bytes = std::stoull(size);
                const char* units[] = {"B", "KB", "MB", "GB", "TB"};
                int unit = 0;
                double humanSize = bytes;
                while (humanSize >= 1024 && unit < 4) {
                    humanSize /= 1024;
                    ++unit;
                }
                std::cout << "Size:        " << humanSize << " " << units[unit] << "\n";
            } catch (...) {
                std::cout << "Size:        " << size << " bytes\n";
            }
            std::cout << "Type:        " << type << "\n";
            std::cout << "Mountpoint:  " << (mountpoint.empty() ? "-" : mountpoint) << "\n";
        }
    }
    if (!found) {
        std::cout << "No disk info found!\n";
    } else {
        if (!mountpoint.empty() && mountpoint != "-") {
            std::string dfcmd = "df -h '" + mountpoint + "' | tail -1";
            std::string dfout = Terminalexec::execTerminal(dfcmd.c_str());
            std::istringstream dfiss(dfout);
            std::string filesystem, size, used, avail, usep, mnt;
            dfiss >> filesystem >> size >> used >> avail >> usep >> mnt;
            std::cout << "Used:        " << used << "\n";
            std::cout << "Available:   " << avail << "\n";
            std::cout << "Used %:      " << usep << "\n";
        } else {
            std::cout << "No mountpoint, cannot show used/free space.\n";
        }
    }
    std::cout << "------------------------------\n";
}


// Dummy implementations for the other functions (to avoid linker errors)
bool formatDrive(const std::string&, const std::string&, const std::string&) { return false; }
bool encryptDrive(const std::string&, bool) { return false; }
bool resizeDrive(const std::string&, int) { return false; }
bool checkDriveHealth(const std::string&) { return false; }

//Format
void formatDrive() {
    std::cout << "\nChoose an option for how to format:\n";
    std::cout << "1. Format drive\n";
    std::cout << "2. Format drive with label\n";
    std::cout << "3. Format drive with label and filesystem\n";
    std::cout << "------------------\n";
    int fdinput;
    std::cin >> fdinput;
    switch (fdinput) {
        case 1:
            std::cout << "Choose a Drive to Format\n";
            {
                std::vector<std::string> drives;
                listDrives(drives);
                if (drives.empty()) break;
                std::cout << "Enter drive number:\n";
                int driveNumber;
                std::cin >> driveNumber;
                if (driveNumber < 0 || driveNumber >= (int)drives.size()) {
                    std::cout << "Invalid selection!\n";
                    break;
                }
                std::cout << "Are you sure you want to format drive " << drives[driveNumber] << "? (y/n):\n";
                std::string confirmationfd;
                std::cin >> confirmationfd;
                if (confirmationfd == "y" || confirmationfd == "Y" ) {
                    std::cout << "Formatting drive: " << drives[driveNumber] << "...\n";
                    std::string cmd = "mkfs.ext4 " + drives[driveNumber];
                    std::string result = Terminalexec::execTerminal(cmd.c_str());
                    if (result.find("error") != std::string::npos) {
                        Logger::log("[ERROR] Failed to format drive: " + drives[driveNumber]);
                        std::cout << "[Error] Failed to format drive: " << drives[driveNumber] << "\n";
                    } else {
                        Logger::log("[INFO] Drive fromated successfully: " + drives[driveNumber]);
                        std::cout << "Drive formatted successfully: " << drives[driveNumber] << "\n";
                    }
                } else {
                    Logger::log("[INFO] Format operation cancelled by user");
                    std::cout << "[Info] Format operation cancelled by user.\n";
                    return;
                }
            }
            break;
        case 2:
            {
                std::vector<std::string> drives;
                listDrives(drives);
                if (drives.empty()) break;
                std::cout << "Enter drive number:\n";
                int driveNumberfd;
                std::cin >> driveNumberfd;
                if (driveNumberfd < 0 || driveNumberfd >= (int)drives.size()) {
                    std::cout << "Invalid selection!\n";
                    break;
                }
                std::cout << "Enter label: ";
                std::string label;
                std::cin >> label;
                std::cout << "Formatting drive with label: " << label << "\n";
                if (label.empty()) {
                    std::cout << "[Error] label cannot be empty!\n";
                    Logger::log("[ERROR] label cannot be empty");
                    return;
                }
                std::cout << "Are you sure you want to format drive " << drives[driveNumberfd] << " with label '" << label << "' ? (y/n)\n";
                char confirmationfd;
                std::cin >> confirmationfd;
                if (confirmationfd != 'y' || confirmationfd != 'Y') {
                    std::cout << "[Info] Formating cancled!\n";
                    Logger::log("[INFO] froamting canceled");
                    break;
                }
                std::string execTerminal(("mkfs.ext4 -L " + label + " " + drives[driveNumberfd]).c_str());
                if (execTerminal.find("error") != std::string::npos) {
                    std::cout << "[Error] Failed to format drive: " << drives[driveNumberfd] << "\n";
                    Logger::log("[ERROR] Failed to format drive: " + drives[driveNumberfd]);
                } else {
                    std::cout << "[INFO] Drive formatted successfully with label: " << label << "\n";
                    Logger::log("[INFO] Drive formatted successfully with label: " + label + " -> foramtDrvie()");
                }
            }
            break;
        case 3:
            {
                std::vector<std::string> drives;
                listDrives(drives);
                if (drives.empty()) break;
                std::cout << "Enter drive number:\n";
                int drivenumberfd3;
                std::cin >> drivenumberfd3;
                if (drivenumberfd3 < 0 || drivenumberfd3 >= (int)drives.size()) {
                    std::cout << "Invalid selection!\n";
                    break;
                }
                std::string label, fsType;
                std::cout << "Enter label: ";
                std::cin >> label;
                std::cout << "Enter filesystem type (e.g. ext4): ";
                std::cin >> fsType;
                std::cout << "Are you sure you want to format drive " << drives[drivenumberfd3] << " with label '" << label << "' and filesystem type '" << fsType << "' ? (y/n)\n";
                char confirmationfd3;
                std::cin >> confirmationfd3;
                if (confirmationfd3 != 'y' || confirmationfd3 != 'Y') {
                    std::cout << "[Info] Formating cancelled!\n";
                    Logger::log("[INFO] Format operation cancelle by user -> formatDrive()");
                    return;
                }
                std::string execTerminal(("mkfs." + fsType + " -L " + label + " " + drives[drivenumberfd3]).c_str());
                if (execTerminal.find("error") != std::string::npos) {
                    std::cout << "[Error] Failed to format drive: " << drives[drivenumberfd3] << "\n";
                    Logger::log("[ERROR] Failed to format drive: " + drives[drivenumberfd3]);
                } else {
                    std::cout << "Drive formatted successfully with label: " << label << " and filesystem type: " << fsType << "\n";
                    Logger::log("[INFO] Drive formatted successfully with label: " + label + " and filesystem type: " + fsType + " -> formatDrive()");
                }
            }
            break;
        default:
            std::cout << "[Error] Invalid option selected.\n";
    }
}

// checkDriveHealth
int checkDriveHealth() {
    std::vector<std::string> drives;
    listDrives(drives);
    if (drives.empty()) {
        std::cout << "No drives found!\n";
        return 1;
    }
    std::cout << "\nEnter drive name of a drive to check Health:\n";
    std::string driveHealth_name;
    std::cin >> driveHealth_name;
    std::string cmd = "smartctl -H " + driveHealth_name + " 2>&1 | grep 'SMART overall-health'";
    std::string output = Terminalexec::execTerminal(cmd.c_str());
    if (output.find("PASSED") != std::string::npos) {
        std::cout << "Drive " << driveHealth_name << " is healthy.\n";
    } else if (output.find("FAILED") != std::string::npos){
        std::cout << "Drive " << driveHealth_name << " has issues.\n";
    } else {
        std::cout << "[Error] Unable to determine drive health. SMART may not be installed or unexpected error occurred\n";
        
    }
   return 1;
}

// resizeDrive
void resizeDrive() {
    std::vector<std::string> drives;
    listDrives(drives);
    if (drives.empty()) {
        std::cout << "No drives found!\n";
        return;
    }
    std::cout << "Enter drive number to resize:\n";
    int driveNumber_resize;
    std::cin >> driveNumber_resize;
    if (driveNumber_resize < 0 || driveNumber_resize >= (int) drives.size()) {
        std::cout << "[Error] Invalid selection!\n";
        return;
    }
    std::cout << "Enter new size in GB for drive " << drives[driveNumber_resize] << ":\n";
    int newSize;
    std::cin >> newSize;
    if (newSize <= 0) {
        std::cout << "[Error] Invalid size entered!\n";
        return;
    }
    std::cout << "Resizing drive " << drives[driveNumber_resize] << " to " << newSize << " GB...\n";
    if (resizeDrive(drives[driveNumber_resize], newSize)) {
        std::cout << "Drive resized successfully\n";
    } else {
        std::cout << "[Error] Failed to resize drive\n";
        Logger::log("[ERROR] Failed to resize drive: " + drives[driveNumber_resize] + " -> resizeDrive()");
    }
}

// encryption
class EnDecryptionUtils {
    public:
        static void saveEncryptionInfo(const EncryptionInfo& info) {
            std::ofstream file(KEY_STORAGE_PATH, std::ios::app | std::ios::binary);
            if (!file) {
                std::cerr << "[Error] Cannot open key storage file\n";
                Logger::log("[ERROR] Cannot open key storage file: " + KEY_STORAGE_PATH + " -> saveEncryptionInfo()");
                return;
            }

            char driveName[256] = {0};
            strncpy(driveName, info.driveName.c_str(), 255);
            file.write(driveName, sizeof(driveName));
            file.write((char*)info.key, sizeof(info.key));
            file.write((char*)info.iv, sizeof(info.iv));
            file.close();
        }

        static bool loadEncryptionInfo(const std::string& driveName, EncryptionInfo& info) {
            std::ifstream file(KEY_STORAGE_PATH, std::ios::binary);
            if (!file) {
                std::cerr << "[Error] Cannot open key storage file\n";
                Logger::log("[ERROR] Cannot open key storage file: " + KEY_STORAGE_PATH + " -> loadEncryptionInfo()");
                return false;
            }
            char storedDriveName[256];
            while (file.read(storedDriveName, sizeof(storedDriveName))) {
                file.read((char*)info.key, sizeof(info.key));
                file.read((char*)info.iv, sizeof(info.iv));
                if (driveName == storedDriveName) {
                    info.driveName = driveName;
                    return true;
                }
            }
            return false;
        }

        static void generateKeyAndIV(unsigned char* key, unsigned char* iv) {
            if (!RAND_bytes(key, 32) || !RAND_bytes(iv, 16)) {
                throw std::runtime_error("[Error] Failed to generate random key/IV");
                Logger::log("[ERROR] Failed to generate random key/IV for encryption -> generateKeyAndIV()");
            }
        }

        static void encryptDrive(const std::string& driveName) {
            EncryptionInfo info;
            info.driveName = driveName;
            generateKeyAndIV(info.key, info.iv);
            saveEncryptionInfo(info);
            std::stringstream ss;
            ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
                << "--key-file <(echo -n '" << std::string((char*)info.key, 32) << "') "
                << "open " << driveName << " encrypted_" << basename(driveName.c_str());
            Logger::log("[INFO] Encrypting drive: " + driveName);
            std::string output = Terminalexec::execTerminal(ss.str().c_str());
            if (output.find("Command failed") != std::string::npos) {
                std::cerr << "[Error] Encryption failed: " << output << "\n";
                Logger::log("[ERROR] Encryption failed for drive: " + driveName + " -> encryptDrive()");
                return;
            }
            std::cout << "Drive encrypted successfully. The decryption key is stored in " << KEY_STORAGE_PATH << "\n";
            Logger::log("[INFO] Drive enrypted successsfully: " + driveName + " -> encryptDrive()");
        }

        static void decryptDrive(const std::string& driveName) {
            EncryptionInfo info;
            if (!loadEncryptionInfo(driveName, info)) {
                std::cerr << "[Error] No encryption key found for " << driveName << "\n";
                Logger::log("[ERROR] No encryption key found for " + driveName + " -> decryptDrive()");
                return;
            }
            // Create dm-crypt mapping for decryption
            std::stringstream ss;
            ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
                << "--key-file <(echo -n '" << std::string((char*)info.key, 32) << "') "
                << "open " << driveName << " decrypted_" << basename(driveName.c_str());
            std::string output = Terminalexec::execTerminalv2(ss.str().c_str());
            if (output.find("Command failed") != std::string::npos) {
                std::cerr << "Decryption failed: " << output << "\n";
                Logger::log("[ERROR] Decryption failed " + output + " -> decryptDrive()");
                return;
            }
            std::cout << "Drive decrypted successfully.\n";
            Logger::log("[INFO] Drive decrypted successfully -> decryptDrive()");
        }

};


void EnDecryptDrive() {
    std::vector<std::string> drives;
    listDrives(drives);
    if (drives.empty()) {
        std::cout << "No drives found!\n";
        return;
    }

    std::cout << "\nEnter the name of the drive to encrypt/decrypt (e.g., /dev/sda):\n";
    std::string driveName;
    std::cin.ignore();
    std::getline(std::cin, driveName);

    bool driveFound = false;
    for (const auto& drive : drives) {
        if (drive == driveName) {
            driveFound = true;
            break;
        }
    }
    if (!driveFound) {
        std::cout << "[Error] Drive " << driveName << " not found!\n";
        Logger::log("[ERROR] Drive " + driveName + " not found -> void EnDecrypt()");
        return;
    }

    std::cout << "Type 'e' to encrypt or 'd' to decrypt: ";
    char endecryptselect;
    std::cin >> endecryptselect;

    if (endecryptselect == 'e' || endecryptselect == 'E') {
        std::cout << "[Warning] Are you sure you want to encrypt " << driveName << "? (y/n)\n";
        char endecryptconfirm;
        std::cin >> endecryptconfirm;
        if (endecryptconfirm != 'y' && endecryptconfirm != 'Y') {
            std::cout << "[Info] Encryption cancelled.\n";
            Logger::log("[INFO] Encryption cancelled -> void EnDecrypt()");
            return;
        }

        char randomconfirmationkey[] = {'a', 'A', 'b', 'B', 'c', 'C', 'd', 'D', 'e', 'E', 'f', 'F', 'g', 'G', 'h', 'H', 'i', 'I', 'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N', 'o', 'O', 'p', 'P', 'q', 'Q', 'r', 'R', 's', 'S', 't', 'T', 'u', 'U', 'v', 'V', 'w', 'W', 'x', 'X', 'y', 'Y', 'z', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        std::string displayKey;
        for (int i = 0; i < 10; i++) {
            int randomIndex = rand() % (sizeof(randomconfirmationkey) / sizeof(randomconfirmationkey[0]));
            displayKey += randomconfirmationkey[randomIndex];
        }

        std::cout << "\n[Security] Confirmation key: " << displayKey;
        std::cout << "\n[Input] Please enter the confirmation key to proceed:\n";
        char userInput[11] = {0};
        std::cin >> userInput;
        
        if (std::string(userInput) != displayKey) {
            std::cout << "[Error] Invalid confirmation key\n";
            Logger::log("[ERROR] Confirmation key invalid -> void EnDecrypt()");
            return;
        }
        
        std::cout << "[Process] Proceeding with encryption of " << driveName << "...\n";
        EncryptionInfo info;
        info.driveName = driveName;
        
        if (!RAND_bytes(info.key, 32) || !RAND_bytes(info.iv, 16)) {
            std::cout << "[Error] Failed to generate encryption key\n";
            Logger::log("[ERROR] Failed to generate encryption key -> void EnDecrypt()");
            return;
        }
        EnDecryptionUtils::loadEncryptionInfo(driveName, info);

        std::cout << "\n[Input] Enter a name for the encrypted device (e.g., encrypted_drive): ";
        std::string deviceNameEncrypt;
        std::cin >> deviceNameEncrypt;
        
        std::stringstream ss;
        ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
           << "--key-file <(echo -n '" << std::string((char*)info.key, 32) << "') "
           << "open " << driveName << " " << deviceNameEncrypt;
        
        std::string output = Terminalexec::execTerminal(ss.str().c_str());
        if (output.find("Command failed") != std::string::npos) {
            std::cout << "[Error] Failed to encrypt the drive: " << output << "\n";
            Logger::log("[ERROR] failed to encrypt teh drive -> void EnDecrypt()");
        } else {
            std::cout << "[Success] Drive " << driveName << " has been encrypted as " << deviceNameEncrypt << "\n";
            std::cout << "[Info] The decryption key is stored in " << KEY_STORAGE_PATH << "\n";
            Logger::log("[INFO] Key saved -> void EnDecrypt()");
        }
    } else if (endecryptselect == 'd' || endecryptselect == 'D') {
        std::cout << "[Warning] Are you sure you want to decrypt " << driveName << "? (y/n)\n";
        char decryptconfirm;
        std::cin >> decryptconfirm;
        if (decryptconfirm != 'y' && decryptconfirm != 'Y') {
            std::cout << "[Info] Decryption cancelled.\n";
            Logger::log("[INFO] Ddecryption cancelled -> void EnDecrypt()");
            return;
        }
        
        char randomconfirmationkey[] = {'a', 'A', 'b', 'B', 'c', 'C', 'd', 'D', 'e', 'E', 'f', 'F', 'g', 'G', 'h', 'H', 'i', 'I', 'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N', 'o', 'O', 'p', 'P', 'q', 'Q', 'r', 'R', 's', 'S', 't', 'T', 'u', 'U', 'v', 'V', 'w', 'W', 'x', 'X', 'y', 'Y', 'z', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        std::string displayKey;
        for (int i = 0; i < 10; i++) {
            int randomIndex = rand() % (sizeof(randomconfirmationkey) / sizeof(randomconfirmationkey[0]));
            displayKey += randomconfirmationkey[randomIndex];
        }

        std::cout << "\n[Security] Confirmation key: " << displayKey;
        std::cout << "\n[Input] Please enter the confirmation key to proceed:\n";
        
        char userInput[11] = {0};
        std::cin >> userInput;
        if (std::string(userInput) != displayKey) {
            std::cout << "[Error] Invalid confirmation key\n";
            Logger::log("[ERROR] invalid confirmation key -> void Endecrypt()");
            return;
        }
        
        std::cout << "[Process] Loading encryption key for " << driveName << "...\n";
        
        EncryptionInfo info;
        if (!EnDecryptionUtils::loadEncryptionInfo(driveName, info)) {
            std::cout << "[Error] No encryption key found for " << driveName << "\n";
            Logger::log("[ERROR] No encryption key found -> void EnDecrypt()");
            return;
        }
        
        std::cout << "Enter the name of the encrypted device to decrypt: ";
        std::string deviceNameDecrypt;
        std::cin >> deviceNameDecrypt;
    
        std::stringstream ss;
        ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
           << "--key-file <(echo -n '" << std::string((char*)info.key, 32) << "') "
           << "open " << driveName << " " << deviceNameDecrypt << "_decrypted";
        
        std::string output = Terminalexec::execTerminal(ss.str().c_str());
        if (output.find("Command failed") != std::string::npos) {
            std::cout << "[Error] Failed to decrypt the drive: " << output << "\n";
            Logger::log("[ERROR] failed to decrypt the drive -> void EnDecrypt()");
        } else {
            std::cout << "[Success] Drive " << driveName << " has been decrypted\n";
        }
    } else {
        std::cout << "[Error] Invalid action! Use 'e' for encrypt or 'd' for decrypt\n";
        return;
    }
} 

// Overwrite drive data
void OverwriteDriveData() {
    std::vector<std::string> drives;
    listDrives(drives);
    if (drives.empty()) {
        std::cout << "No drives found!\n";
        return;
    }
    std::cout << "Enter the name of an Drive you want to overwrite:\n";
    std::string driveName;
    std::cin >> driveName;
    std::cout << "Are you sure you want to overwrite the drive " << driveName << "? (y/n)\n";
    char confirmationzerodrive;
    std::cin >> confirmationzerodrive;
    if (confirmationzerodrive == 'y' || confirmationzerodrive == 'Y' ) {
        char randomconfirmationkey[] = {'a', 'A', 'b', 'B', 'c', 'C', 'd', 'D', 'e', 'E', 'f', 'F', 'g', 'G', 'h', 'H', 'i', 'I', 'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N', 'o', 'O', 'p', 'P', 'q', 'Q', 'r', 'R', 's', 'S', 't', 'T', 'u', 'U', 'v', 'V', 'w', 'W', 'x', 'X', 'y', 'Y', 'z', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        for (int i = 0; i < 10; i++) {
            int randomIndex = rand() % (sizeof(randomconfirmationkey) / sizeof(randomconfirmationkey[0]));
            std::cout << randomconfirmationkey[randomIndex];
        }
        std::cout << "\nPlease enter the confirmationkey to preceed with the operation:\n";
        std::cout << randomconfirmationkey << "\n";
        char randomconfirmationkeyinput[10];
        std::cin >> randomconfirmationkeyinput;
        if (std::string(randomconfirmationkeyinput) != std::string(randomconfirmationkey)) {
            std::cout << "[Error] Invalid confrimation of the Key or unexpected error\n";
            Logger::log("[ERROR] Invalid confrimation of the Key or unexpected error -> OverwriteData");
        }
        std::cout << "Proceeding with Overwriting " << driveName << "...\n";
        std::string devrandom = Terminalexec::execTerminalv2("sudo dd if=/dev/urandom of=" + driveName + " bs=1M status=progress");
        std::string devZero = Terminalexec::execTerminalv2("sudo dd if=/dev/zero of=" + driveName + " bs=1M status=progress");
        if (devZero.empty() && devrandom.empty()) {
            std::cout << "[Error] Failed to overwrite the drive\n";
        } else if (devZero.empty() || devrandom.empty()) {
            std::cout << "[Warning] the drive was not completely overwriten, please check the drive and try again if necessary\n";
        } else {
            std::cout << driveName << "'s data has been overwriten successfully\n";
        }
    } else {
        std::cout << "[Info] Overwriting cancelled\n";
    }
}

// medatareader
class MetadataReader {
private:
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

    static DriveMetadata getMetadata(const std::string& drive) {
        DriveMetadata metadata;
        std::string cmd = "lsblk -J -o NAME,SIZE,MODEL,SERIAL,TYPE,MOUNTPOINT,VENDOR,FSTYPE,UUID -p " + drive;
        std::string json = Terminalexec::execTerminalv3(cmd);
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

    static void displayMetadata(const DriveMetadata& metadata) {
        std::cout << "\n-------- Drive Metadata --------\n";
        std::cout << "Name:       " << metadata.name << "\n";
        std::cout << "Size:       " << metadata.size << "\n";
        std::cout << "Model:      " << (metadata.model.empty() ? "N/A" : metadata.model) << "\n";
        std::cout << "Serial:     " << (metadata.serial.empty() ? "N/A" : metadata.serial) << "\n";
        std::cout << "Type:       " << metadata.type << "\n";
        std::cout << "Mountpoint: " << (metadata.mountpoint.empty() ? "Not mounted" : metadata.mountpoint) << "\n";
        std::cout << "Vendor:     " << (metadata.vendor.empty() ? "N/A" : metadata.vendor) << "\n";
        std::cout << "Filesystem: " << (metadata.fstype.empty() ? "N/A" : metadata.fstype) << "\n";
        std::cout << "UUID:       " << (metadata.uuid.empty() ? "N/A" : metadata.uuid) << "\n";

        // SMART data
        if (metadata.type == "disk") {
            std::cout << "\n-------- SMART Data --------\n";
            std::string smartCmd = "smartctl -i " + metadata.name;
            std::string smartOutput = Terminalexec::execTerminal(smartCmd.c_str());
            if (!smartOutput.empty()) {
                std::cout << smartOutput;
            } else {
                std::cout << "SMART data not available/intalled\n";
            }
        }
        std::cout << "------------------------------\n";
    }
    
public:
    static void mainReader() {
        std::vector<std::string> drives;
        listDrives(drives);
        if (drives.empty()) {
            std::cout << "[Error] No drives found!\n";
            Logger::log("[ERROR] No drives found -> MetadataReader::mainReader()");
            return;
        }
        std::cout << "\nEnter number of the drive you want to see the metadata of: ";
        int driveNum;
        std::cin >> driveNum;
        if (driveNum < 0 || driveNum >= (int)drives.size()) {
            std::cout << "[Error] Invalid drive selection!\n";
            Logger::log("[ERROR] Invalid drive selection in MetadataReader");
            return;
        }
        try {
            DriveMetadata metadata = getMetadata(drives[driveNum]);
            displayMetadata(metadata);
            Logger::log("[INFO] Successfully read metadata for drive: " + drives[driveNum]);
        } catch (const std::exception& e) {
            std::cout << "[Error] Failed to read drive metadata: " << e.what() << "\n";
            Logger::log("[ERROR] Failed to read drive metadata: " + std::string(e.what()));
        }
    }
};


class MountUtility {
private:
    static void BurnISOToStorageDevice() { //const std::string& isoPath, const std::string& drive
        std::vector<std::string> drives;
        listDrives(drives);
        if (drives.empty()) {
            std::cout << "[Error] No drives found\n";
            return;
        }
        std::cout << "\nEnter the name of a drive to select for burning an iso/img:\n";
        std::string DriveName;
        std::cin >> DriveName;
        std::cout << "Are you sure you want to select " << DriveName << " for this operation? (y/n)\n";
        char confirmationburn;
        std::cin >> confirmationburn;
        if (confirmationburn != 'y' && confirmationburn != 'Y') {
            std::cout << "[Info] Opeartion cancelled\n";
            Logger::log("[INFO] Operation cancelled -> BurnISOToStorageDevice()");
            return;
        }
        std::cout << "\nEnter the path to the iso/img file you want to burn on " << DriveName << ":\n";
        std::string isoPath;
        std::cin >> isoPath;
        std::cout << "Are you sure you want to burn " << isoPath << " to " << DriveName << "? (y/n)\n";
        char confrimationburn2;
        std::cin >> confrimationburn2;
        if (confrimationburn2 != 'y' && confrimationburn2 != 'Y') {
            std::cout << "[Info] Operation cancelled\n";
            Logger::log("[INFO] Operation cancelled -> BurnISOToSotorageDevice()");
            return;
        }
        char randomconfirmationkey[] = {'a', 'A', 'b', 'B', 'c', 'C', 'd', 'D', 'e', 'E', 'f', 'F', 'g', 'G', 'h', 'H', 'i', 'I', 'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N', 'o', 'O', 'p', 'P', 'q', 'Q', 'r', 'R', 's', 'S', 't', 'T', 'u', 'U', 'v', 'V', 'w', 'W', 'x', 'X', 'y', 'Y', 'z', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        for (int i = 0; i < 10; i++) {
            int randomIndex = rand() % (sizeof(randomconfirmationkey) / sizeof(randomconfirmationkey[0]));
            std::cout << randomconfirmationkey[randomIndex];
        }
        std::cout << "\nPlease enter the confirmationkey to preceed with the operation:\n";
        std::cout << randomconfirmationkey << "\n";
        char randomconfirmationkeyinput[10];
        std::cin >> randomconfirmationkeyinput;
        if (std::string(randomconfirmationkeyinput) != std::string(randomconfirmationkey)) {
            std::cout << "[Error] Invalid confrimation of the Key or unexpected error\n";
            Logger::log("[ERROR] Invalid confrimation of the Key or unexpected error -> OverwriteData");
            return;
        } else {
            try {
                std::cout << "Proceeding with burning " << isoPath << " to " << DriveName << "...\n";
                std::string bruncmd = "sudo dd if=" + isoPath + " of=" + DriveName + " bs=4M status=progress && sync";
                std::string brunoutput = Terminalexec::execTerminalv2(bruncmd.c_str());
                if (brunoutput.find("error") != std::string::npos) {
                    Logger::log("[ERROR] Failed to burn iso/img to drive: " + DriveName + " -> BurnISOToStorageDevice()"); 
                    throw std::runtime_error("[Error] Faile to burn iso/img to drive: " + DriveName);
                }
                std::cout << "[Success] Successfully burned " << isoPath << " to " << DriveName << "\n";
                Logger::log("[INFO] Successfully burned iso/img to drive: " + DriveName + " -> BurnISOToStorageDevice()");
            } catch (const std::exception& e) {
                std::cout << e.what() << "\n";
            } 
        }
    }

    static void MountDrive2() {
        std::vector<std::string> drives;
        listDrives(drives);
        if (drives.empty()) {
            std::cout << "[Error] No dirves found\n";
            return;
        }
        std::cout << "\nEnter the number of a drive you want to mount:\n";
        int drivemountnum;
        std::cin >> drivemountnum;
        if (drivemountnum < 0 || drivemountnum >= drives.size()) {
            std::cout << "[Error] Invalid selection\n";
            return;
        }
        std::string mountpoint = "sudo mount " + drives[drivemountnum] + " /mnt/" + basename(drives[drivemountnum].c_str());
        std::string mountoutput = Terminalexec::execTerminalv2(mountpoint.c_str());
        if (mountoutput.find("error") != std::string::npos) {
            std::cout << "[Error] Failed to mount drive: " << mountoutput << "\n";
            Logger::log("[ERROR] Failed to mount drive: " + drives[drivemountnum] + " -> MountDrive()");
            return;
        }
    }

    static void UnmountDrive2() {
        std::vector<std::string> drives;
        listDrives(drives);
        if (drives.empty()) {
            std::cout << "[Error] No drives found\n";
            return;
        }
        std::cout << "\nEnter the number of a drive you want to unmount:\n";
        int driveunmountnum;
        std::cin >> driveunmountnum;
        if (driveunmountnum < 0 || driveunmountnum >= drives.size()) {
            std::cout << "[Error] Invalid selection\n";
            return;
        }
        std::string unmountpoint = "sudo umount " + drives[driveunmountnum];
        std::string unmountoutput = Terminalexec::execTerminalv2(unmountpoint.c_str());
        if (unmountoutput.find("error") != std::string::npos) {
            std::cout << "[Error] Failed to unmount drive: " << unmountoutput << "\n";
            Logger::log("[ERROR] Failed to unmount drive: " + drives[driveunmountnum] + " -> UnmountDrive()");
            return;
        }
    }


    static int ExitReturn(bool& running) {
        running = false;
        return 0;   
    }

public:
    static void mainMountUtil() {
        enum MenuOptions {
            Burniso = 1, MountDrive = 2, UnmountDrive = 3, Exit = 0
        };

        std::cout << "\n--------- Mount menu ---------\n";
        std::cout << "1. Burn iso/img to storage device\n";
        std::cout << "2. Mount storage device\n";
        std::cout << "3. Unmount storage device\n";
        std::cout << "0. Exit/Rreturn to main menu\n";
        std::cout << "--------------------------------\n";
        int menuinputmount;
        std::cin >> menuinputmount;
        switch (menuinputmount) {
            case Burniso: {
                BurnISOToStorageDevice();
                break;
            }
            case MountDrive: {
                MountDrive2();
                break;
            }
            case UnmountDrive: {
                UnmountDrive2();
                break;
            }
            case Exit: {
                return;
            }
            default:
                std::cout << "[Error] Invalid selection\n";
                return;
        }
    }
};

class ForensicAnalysis {
private:
    static void CreateDiskImage() {
        std::vector<std::string> drives;
        listDrives(drives);
        if (drives.empty()) {
            std::cout << "[Error] No drives found\n";
            return;
        }
        std::cout << "\nEnter the name of a drive to create a disk image (e.g., /dev/sda):\n";
        std::string drive;
        std::cin >> drive;
        bool driveFound = false;
        for (const auto& d : drives) {
            if (d == drive) {
                driveFound = true;
                break;
            }
        }
        if (!driveFound) {
            std::cout << "[Error] Drive " << drive << " not found!\n";
            Logger::log("[ERROR] Drive " + drive + " not found -> CreateDiskImage()");
            return;
        }
        std::cout << "Enter the path where the disk image should be saved (e.g., /path/to/image.img):\n";
        std::string imagePath;
        std::cin >> imagePath;
        std::cout << "Are you sure you want to create a disk image of " << drive << " at " << imagePath << "? (y/n)\n";
        char confirmationcreate;
        std::cin >> confirmationcreate;
        if (confirmationcreate != 'y' && confirmationcreate != 'Y') {
            std::cout << "[Info] Operation cancelled\n";
            Logger::log("[INFO] Operation cancelled -> CreateDiskImage()");
            return;
        }
        std::string cmd = "sudo dd if=" + drive + " of=" + imagePath + " bs=4M status=progress && sync";
        std::string output = Terminalexec::execTerminalv2(cmd.c_str());
        if (output.find("error") != std::string::npos) {
            std::cout << "[Error] Failed to create disk image: " << output << "\n";
            Logger::log("[ERROR] Failed to create disk image for drive: " + drive + " -> CreateDiskImage()");
            return;
        }
        std::cout << "[Success] Disk image created at " << imagePath << "\n";
        Logger::log("[INFO] Disk image created successfully for drive: " + drive + " -> CreateDiskImage()");
    }

    static void ScanDrive() {
        std::cout << "\n-------- Scan Drive for Recoverable Files ---------\n";
        std::cout << "1. files recovery\n";
        std::cout << "2. partition recovery\n";
        std::cout << "3. system recovery\n";
        std::cout << "-----------------------------------------------------\n";
        std::cout << "In development...\n";

    }
public:
    static int mainForensic(bool& running) {
        enum ForensicMenuOptions {
            Info = 1, CreateDisktImage = 2, ScanDrive = 3, Exit_Return = 0
        };
        std::cout << "\n-------- Forensic Analysis menu ---------\n";
        std::cout << "1. Info of the Analysis tool\n";
        std::cout << "2. Create a disk image of a drive\n";
        std::cout << "3. Scan drive for recoverable files\n";
        std::cout << "In development...\n";
        std::cout << "0. Exit/Return to the main menu\n";
        std::cout << "-------------------------------------------\n";
        int forsensicmenuinput;
        std::cin >> forsensicmenuinput;
        switch (static_cast<ForensicMenuOptions>(forsensicmenuinput)) {
            case Info:
                std::cout << "\n[Info] This is a custom made forensic analysis tool for the Drive Manager\n";
                std::cout << "Its not using actual forsensic tools, but still if its finished would be fully functional\n";
                std::cout << "In development...\n";
                break;
            case CreateDisktImage:
                CreateDiskImage();
                break;
            case ScanDrive:
                std::cout << "[Info] In development...\n";
                break;
            case Exit_Return:
                std::cout << "\nDo you want to return to the main menu or exit? (r/e)\n";
                char exitreturninput;
                std::cin >> exitreturninput;
                if (exitreturninput == 'r' || exitreturninput == 'R') return;
                else if (exitreturninput == 'e' || exitreturninput == 'E') {
                    running = false;
                    return 0;
                }
                return;
            default:
                std::cout << "[Error] Invalid selection\n";
                return;
        }
    }
};


// main and Info
void Info() {
    std::cout << "\n----------- Info -----------\n";
    std::cout << "Welcome to Drive Manager, this is a porgram for linux to view, operate,... your Drives in your system\n";
    std::cout << "Warning! You should know some basic things about drives so you dont loose any data\n";
    std::cout << "If you found any problems, visit my Github page and send an issue template\n";
    std::cout << "Basic info:\n";
    std::cout << "Version: 0.8.88-18\n";
    std::cout << "Github: https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux\n";
    std::cout << "Author: Dogwalker-kryt\n";
    std::cout << "----------------------------\n";
}

void MenuQues(bool& running) {   
    std::cout << "\nPress '1' for returning to the main menu, '2' to exit\n";
    int menuques;
    std::cin >> menuques;
    if (menuques == 1) {
        running = true;
    } else if (menuques == 2) {
        running = false;
    } else {
        std::cout << "[Error] Wrong input\n";
        running = true; 
    }
}
enum MenuOptionsMain {
    EXITPROGRAM = 0, LISTDRIVES = 1, FORMATDRIVE = 2, ENCRYPTDECRYPTDRIVE = 3, RESIZEDRIVE = 4, 
    CHECKDRIVEHEALTH = 5, ANALYZEDISKSPACE = 6, OVERWRITEDRIVEDATA = 7, VIEWMETADATA = 8, VIEWINFO = 9, MOUNTUNMOUNT = 10
};

int main() {
    bool running = true;
    while (running == true) {
        std::string clear = Terminalexec::execTerminal("clear");
        std::cout << clear;
        std::cout << "\nWelcome to Drive-Manager\n";
        std::cout << "------------- Menu -------------\n";
        std::cout << "1. List drives\n";
        std::cout << "2. Format drive\n";
        std::cout << "3. Encrypt/Decrypt drive with AES-256\n";
        std::cout << "4. Resize drive\n";
        std::cout << "5. Check drive health\n";
        std::cout << "6. Analyze Disk Space\n";
        std::cout << "7. Overwrite Drive Data\n";
        std::cout << "8. View Metadata of a Drive\n";
        std::cout << "9. View Info\n";
        std::cout << "10. Mount/Unmount iso's, Drives,... (in development)\n";
        std::cout << "11. Forensic analysis (in development)\n";
        std::cout << "0. Exit\n";
        std::cout << "--------------------------------\n";
        int menuinput;
        std::cin >> menuinput;
        switch (static_cast<MenuOptionsMain>(menuinput)) {
            case LISTDRIVES: {
                std::vector<std::string> drives;
                listDrives(drives);
                std::cout << "\nPress '1' to return, '2' for advanced listing, or '3' to exit\n";
                int menuques2;
                std::cin >> menuques2;
                if (menuques2 == 1) continue;
                else if (menuques2 == 2) listpartisions(drives);
                else if (menuques2 == 3) running = false;
                break;
            }
            case FORMATDRIVE:
                formatDrive();
                MenuQues(running);
                break;
            case ENCRYPTDECRYPTDRIVE:
                EnDecryptDrive();
                MenuQues(running);
                break;
            case RESIZEDRIVE:
                resizeDrive();
                MenuQues(running);
                break;
            case CHECKDRIVEHEALTH:
                checkDriveHealth();
                MenuQues(running);
                break;
            case ANALYZEDISKSPACE:
                analyDiskSpace();
                MenuQues(running);
                break;
            case OVERWRITEDRIVEDATA:
                std::cout << "[Warning] This function will overwrite the entire data to zeros. Proceed? (y/n)\n";
                char zerodriveinput;
                std::cin >> zerodriveinput;
                if (zerodriveinput == 'y' || zerodriveinput == 'Y') OverwriteDriveData();
                break;
            case VIEWMETADATA:
                MetadataReader::mainReader();
                MenuQues(running);
                break;
            case VIEWINFO:
                Info();
                MenuQues(running);
                break;
            case MOUNTUNMOUNT:
                MountUtility::mainMountUtil();
                break;
            case EXITPROGRAM:
                running = false;
                break;
            default:
                std::cout << "[Error] Invalid selection\n";
                break;
        }
    }
    return 0;
}
