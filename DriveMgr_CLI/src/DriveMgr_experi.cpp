/* −··− x f e t r o j k
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
// v0.8.90

// standard c++ libarys, i think
#include <iostream>
#include <cstdlib>
#include <regex>
#include <cstdio>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <array>
#include <limits>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <fstream>
#include <regex>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <map>
#include <cstdint>
#include <fcntl.h>
// openssl
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
// custom .h
#include "../include/drivefunctions.h"

// Forward declarations for free-form recovery helpers implemented in filerecoevry.cpp
// Placed at file scope so class members can call them without modifying headers.
void file_recovery_quick(const std::string& drive, const std::string& signature_key);
void file_recovery_full(const std::string& drive, const std::string& signature_key);


// checksfielsytem
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

// analyzeDriskapce··−· 
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
    try {
        std::string cmd = "sudo smartctl -H " + driveHealth_name;
        std::string output = Terminalexec::execTerminal(cmd.c_str());
        std::cout << output;
    }
    catch(const std::exception& e) {
        std::string error = e.what();
        Logger::log("[ERROR]" + error);
        throw std::runtime_error(e.what());
    }
   return 0;
}

// resizeDrive
void resizeDrive() {
    std::cout << "\nResizing\n";
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

            auto salt = generateSalt();
            auto obfKey = obfuscate(info.key, sizeof(info.key), salt.data(), salt.size());
            auto obfIV = obfuscate(info.iv, sizeof(info.iv), salt.data(), salt.size());

            // Write drive name (null-terminated)
            char driveName[256] = {0};
            strncpy(driveName, info.driveName.c_str(), 255);
            file.write(driveName, sizeof(driveName));
            uint32_t saltLen = salt.size();
            file.write(reinterpret_cast<const char*>(&saltLen), sizeof(saltLen));
            file.write(reinterpret_cast<const char*>(salt.data()), salt.size());
            //·  − Write obfuscated key and IV
            file.write(reinterpret_cast<const char*>(obfKey.data()), obfKey.size());
            file.write(reinterpret_cast<const char*>(obfIV.data()), obfIV.size());

            file.close();
            Logger::log("[INFO] Encryption info saved (salted and obfuscated) for: " + info.driveName);
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
                // Read salt length and salt
                uint32_t saltLen;
                file.read(reinterpret_cast<char*>(&saltLen), sizeof(saltLen));
                std::vector<unsigned char> salt(saltLen);
                file.read(reinterpret_cast<char*>(salt.data()), saltLen);

                // Read obfuscated key and IV
                std::vector<unsigned char> obfKey(32);
                std::vector<unsigned char> obfIV(16);
                file.read(reinterpret_cast<char*>(obfKey.data()), 32);
                file.read(reinterpret_cast<char*>(obfIV.data()), 16);

                // Deobfuscate
                auto key = deobfuscate(obfKey.data(), 32, salt.data(), saltLen);
                auto iv = deobfuscate(obfIV.data(), 16, salt.data(), saltLen);

                if (driveName == storedDriveName) {
                    std::copy(key.begin(), key.end(), info.key);
                    std::copy(iv.begin(), iv.end(), info.iv);
                    info.driveName = driveName;
                    Logger::log("[INFO] Encryption info loaded and deobfuscated for: " + driveName);
                    return true;
                }
            }
            Logger::log("[ERROR] No encryption info found for: " + driveName);
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
            // Create dm-crypt·−· mapping for decryption
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

        // salting
        static std::vector<unsigned char> generateSalt(size_t length = 16) {
            std::vector<unsigned char> salt(length);
            if (!RAND_bytes(salt.data(), length)) {
                throw std::runtime_error("[Error] Failed to genearte salt");
                Logger::log("[ERROR] Failed to generate salt");
            }
        }

        static std::vector<unsigned char> obfuscate(const unsigned char* data, size_t dataLen, const unsigned char* salt, size_t saltLen) {
            std::vector<unsigned char> result(dataLen);
            for (size_t i = 0; i < dataLen; ++i) {
                result[i] = data[i] ^ salt[i % saltLen];
            }
            return result;
        }

        static std::vector<unsigned char> deobfuscate(const unsigned char* data, size_t dataLen, const unsigned char* salt, size_t saltLen) {
            return obfuscate(data, dataLen, salt, saltLen);
        }
        
};


void EnDecryptDrive() {
    std::cout << "\nEn- Decryption\n";
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

        srand(static_cast<unsigned int>(time(0))); 
        char randomconfirmationkey[] = {'a', 'A', 'b', 'B', 'c', 'C', 'd', 'D', 'e', 'E', 'f', 'F', 'g', 'G', 'h', 'H', 'i', 'I', 'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N', 'o', 'O', 'p', 'P', 'q', 'Q', 'r', 'R', 's', 'S', 't', 'T', 'u', 'U', 'v', 'V', 'w', 'W', 'x', 'X', 'y', 'Y', 'z', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        std::string displayKey;
        for (int i = 0; i < 10; i++) {
            int randomIndex = rand() % (sizeof(randomconfirmationkey) / sizeof(randomconfirmationkey[0]));
            displayKey += randomconfirmationkey[randomIndex];
        }
        std::cout << "\nPlease enter the confirmationkey to preceed with the operation:\n";
        std::cout << displayKey << "\n";
        std::string randomconfirmationkeyinput3;
        std::cin >> randomconfirmationkeyinput3;
        if (randomconfirmationkeyinput3 != displayKey) {
            std::cout << "[Error] Invalid confrimation of the Key or unexpected error\n";
            Logger::log("[ERROR] Invalid confrimation of the Key or unexpected error -> EnDecrypt");
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
        
        srand(static_cast<unsigned int>(time(0))); 
        char randomconfirmationkey[] = {'a', 'A', 'b', 'B', 'c', 'C', 'd', 'D', 'e', 'E', 'f', 'F', 'g', 'G', 'h', 'H', 'i', 'I', 'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N', 'o', 'O', 'p', 'P', 'q', 'Q', 'r', 'R', 's', 'S', 't', 'T', 'u', 'U', 'v', 'V', 'w', 'W', 'x', 'X', 'y', 'Y', 'z', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        std::string displayKey;
        for (int i = 0; i < 10; i++) {
            int randomIndex = rand() % (sizeof(randomconfirmationkey) / sizeof(randomconfirmationkey[0]));
            displayKey += randomconfirmationkey[randomIndex];
        }
        std::cout << "\nPlease enter the confirmationkey to preceed with the operation:\n";
        std::cout << displayKey << "\n";
        std::string randomconfirmationkeyinput2;
        std::cin >> randomconfirmationkeyinput2;
        if (randomconfirmationkeyinput2 != displayKey) {
            std::cout << "[Error] Invalid confrimation of the Key or unexpected error\n";
            Logger::log("[ERROR] Invalid confrimation of the Key or unexpected error -> EnDecryptDrive");
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
            return;
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
    std::cout << "\nOverwriting Data\n";
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
    char confirmation_zero_drive;
    std::cin >> confirmation_zero_drive;
    if (confirmation_zero_drive == 'y' || confirmation_zero_drive == 'Y' ) {
        srand(static_cast<unsigned int>(time(0))); 
        char random_confirmation_key[] = {'a', 'A', 'b', 'B', 'c', 'C', 'd', 'D', 'e', 'E', 'f', 'F', 'g', 'G', 'h', 'H', 'i', 'I', 'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N', 'o', 'O', 'p', 'P', 'q', 'Q', 'r', 'R', 's', 'S', 't', 'T', 'u', 'U', 'v', 'V', 'w', 'W', 'x', 'X', 'y', 'Y', 'z', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        std::string displayKey;
        for (int i = 0; i < 10; i++) {
            int randomIndex = rand() % (sizeof(random_confirmation_key) / sizeof(random_confirmation_key[0]));
            displayKey += random_confirmation_key[randomIndex];
        }
        std::cout << "\nPlease enter the confirmationkey to preceed with the operation:\n";
        std::cout << displayKey << "\n";
        std::string random_confirmation_key_input;
        std::cin >> random_confirmation_key_input;
        if (random_confirmation_key_input != displayKey) {
            std::cout << "[Error] Invalid confrimation of the Key or unexpected error\n";
            Logger::log("[ERROR] Invalid confrimation of the Key or unexpected error -> OverwriteData");
            return;
        }
        std::cout << "Proceeding with Overwriting " << driveName << "...\n";
        try {
            std::string devrandom = Terminalexec::execTerminalv2("sudo dd if=/dev/urandom of=" + driveName + " bs=1M status=progress");
            std::string devZero = Terminalexec::execTerminalv2("sudo dd if=/dev/zero of=" + driveName + " bs=1M status=progress");
            if (devZero.find("error") != std::string::npos && devrandom.find("error") != std::string::npos) {
                Logger::log("[ERROR] failed to overwrite drive data\n");
                throw std::runtime_error("[Error] Failed to Overwrite drive data");
            } else if (devZero.find("error") != std::string::npos || devrandom.find("error") != std::string::npos) {
                std::cout << "[Warning] failed to Overwrite, only one of two overwriting opeartions succeded\n"
                          << "Try again";
                Logger::log("[WARNING] failed to overwrite, only one overwriting operation succeded");
                return;
            } else if (devZero.find("failed") != std::string::npos || devrandom.find("failed")) {
                std::cout << "[Info] dd command appears to have failed, but the drive may has been writen to zeros!\n";
                return;
            } else if (devZero.find("dd") != std::string::npos || devrandom.find("dd") != std::string::npos) {
                std::cout << "[Info] dd command appears to have failed, but the drive may has been writen to zeros!\n";
                return;
            } else {
                std::cout << "Overwriting completed, all bytes on your drive: " << driveName << " is overwrting to 0\n";
                return;
            }
        }
        catch(const std::exception& e) {
            std::cerr << "[ERROR] " << e.what() << "\n";
            Logger::log("[ERROR] Overwrite failed for drive: " + driveName + " Reason: " + e.what());
        }
        
    } else {
        std::cout << "[Info] Overwriting cancelled\n";
        Logger::log("[INFO] Overwrting cancelled");
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
        std::cout << "| Name:       " << metadata.name << "\n";
        std::cout << "| Size:       " << metadata.size << "\n";
        std::cout << "| Model:      " << (metadata.model.empty() ? "N/A" : metadata.model) << "\n";
        std::cout << "| Serial:     " << (metadata.serial.empty() ? "N/A" : metadata.serial) << "\n";
        std::cout << "| Type:       " << metadata.type << "\n";
        std::cout << "| Mountpoint: " << (metadata.mountpoint.empty() ? "Not mounted" : metadata.mountpoint) << "\n";
        std::cout << "| Vendor:     " << (metadata.vendor.empty() ? "N/A" : metadata.vendor) << "\n";
        std::cout << "| Filesystem: " << (metadata.fstype.empty() ? "N/A" : metadata.fstype) << "\n";
        std::cout << "| UUID:       " << (metadata.uuid.empty() ? "N/A" : metadata.uuid) << "\n";

        // SMART data
        if (metadata.type == "disk") {
            std::cout << "\n┌-─-─-─- SMART Data -─-─-─-─\n";
            std::string smartCmd = "sudo smartctl -i " + metadata.name;
            std::string smartOutput = Terminalexec::execTerminal(smartCmd.c_str());
            if (!smartOutput.empty()) {
                std::cout << smartOutput;
            } else {
                std::cout << "SMART data not available/intalled\n";
            }
        }                                           
        std::cout << "└─  - -─ --- ─ - -─-  - ──- ──- ───────────────────\n";
                    
    }
    
public:
    static void mainReader() {
        std::cout << "\nMedata reader\n";
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
        std::cout << "\nBurn Image to Drive\n";
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
        std::cout << "\n[Mount] Enter the number of a drive you want to mount:\n";
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
        std::cout << "\n[Unmount] Enter the number of a drive you want to unmount:\n";
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

    static void Restore_USB_Drive() {
        std::vector<std::string> drives;
        listDrives(drives);
        if (drives.empty()) {
            std::cout << "[Error] No drives found\n";
            return;
        }
        std::cout << "\n[Restore] Enter the name of a USB device you want to restore from an ISO\n";
        std::string restore_device_name;
        std::cin >> restore_device_name;
        if (std::find(drives.begin(), drives.end(), restore_device_name) == drives.end()) {
            std::cout << "[Error] Drive " << restore_device_name << " not found!\n";
            Logger::log("[ERROR] Drive " + restore_device_name + " not found -> Restore_USB_Drive()");
            return;
        }
        std::cout << "Are you sure you want to overwrite/clean the ISO/Disk_Image from: " << restore_device_name << " ? [y/N]\n";
        char confirm = 'n';
        std::cin >> confirm;
        if (std::tolower(confirm) != 'y') {
            std::cout << "Restore process aborted\n";
            return;
        }
        std::string unmount_cmd = "sudo umount " + restore_device_name + "* 2>/dev/null || true";
        Terminalexec::execTerminalv2(unmount_cmd.c_str());
        std::string wipefs_cmd = "sudo wipefs -a " + restore_device_name + " 2>&1";
        std::string wipefs_out = Terminalexec::execTerminalv2(wipefs_cmd.c_str());
        std::string dd_cmd = "sudo dd if=/dev/zero of=" + restore_device_name + " bs=1M count=10 status=progress && sudo sync";
        std::string dd_out = Terminalexec::execTerminalv2(dd_cmd.c_str());
        if (dd_out.find("error") != std::string::npos || dd_out.find("failed") != std::string::npos) {
            Logger::log("[ERROR] Failed to overwrite the iso image on the usb -> Restore_USB_Drive()");
            std::cerr << "[Error] Failed to overwrite device: " << restore_device_name << "\n";
            return;
        }
        std::string parted_cmd = "sudo parted -s " + restore_device_name + " mklabel msdos mkpart primary 1MiB 100%";
        std::string parted_out = Terminalexec::execTerminalv2(parted_cmd.c_str());
        std::string partprobe_cmd = "sudo partprobe " + restore_device_name + " 2>&1";
        std::string partprobe_out = Terminalexec::execTerminalv2(partprobe_cmd.c_str());
        std::string partition_path = restore_device_name;
        if (!partition_path.empty() && std::isdigit(partition_path.back())) partition_path += "p1"; else partition_path += "1";
        std::string mkfs_cmd = "sudo mkfs.vfat -F32 " + partition_path + " 2>&1";
        std::string mkfs_out = Terminalexec::execTerminalv2(mkfs_cmd.c_str());
        if (parted_out.find("error") != std::string::npos || partprobe_out.find("error") != std::string::npos || mkfs_out.find("error") != std::string::npos) {
            Logger::log("[ERROR] Failed while restoring USB: " + restore_device_name);
            std::cerr << "[Error] One or more steps failed while restoring device. Check output.\n";
            return;
        }
        std::cout << "[Success] Your USB should now function as a normal FAT32 drive (partition: " << partition_path << ")\n";
        Logger::log("[INFO] Restored USB device " + restore_device_name + " -> formatted " + partition_path);
        return;
    }

    static int ExitReturn(bool& running) {
        running = false;
        return 0;   
    }

public:
    static void mainMountUtil() {
        enum MenuOptions {
            Burniso = 1, MountDrive = 2, UnmountDrive = 3, Exit = 0, RESTOREUSB = 4
        };

        std::cout << "\n--------- Mount menu ---------\n";
        std::cout << "1. Burn iso/img to storage device\n";
        std::cout << "2. Mount storage device\n";
        std::cout << "3. Unmount storage device\n";
        std::cout << "4. Restore usb from iso\n";
        std::cout << "0. Rreturn to main menu\n";
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
            case RESTOREUSB: {
                Restore_USB_Drive;
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
    static int exit() {
        return 0;
    }
    static void CreateDiskImage() {
        std::vector<std::string> drives;
        listDrives(drives);
        if (drives.empty()) {
            std::cout << "[Error] No drives found\n";
            return;
        }
        std::cout << "\n[Drive_image_creation] Enter the name of a drive to create a disk image (e.g., /dev/sda):\n";
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

    // recoverymain + side functions
    
    static void recovery() {
        std::cout << "\n-------- Recovery --------- −−−\n";
        std::cout << "1. files recovery\n";
        //std::cout << "2. partition recovery\n";
        std::cout << "3. system recovery\n";
        std::cout << "-----------------------------\n";
        std::cout << "In development...\n";
        int scanDriverecover;
        std::cin >> scanDriverecover;
        switch (scanDriverecover) {
            case 1:
                filerecovery();
                break;
            //case 2:
                //partitionrecovery();
                //break;
            case 3:
                systemrecovery();
                break;
            default:
                std::cout << "[Error] invalid input\n";       
        }
    }
    //·−−− recovery side functions

    // static std::map<std::string, file_signature> signatures ={
    //     {"png",  {"png",  {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}}},
    //     {"jpg",  {"jpg",  {0xFF, 0xD8, 0xFF}}},
    //     {"elf",  {"elf",  {0x7F, 0x45, 0x4C, 0x46}}},
    //     {"zip",  {"zip",  {0x50, 0x4B, 0x03, 0x04}}},
    //     {"pdf",  {"pdf",  {0x25, 0x50, 0x44, 0x46, 0x2D}}},
    //     {"mp3",  {"mp3",  {0x49, 0x44, 0x33}}},
    //     {"mp4",  {"mp4",  {0x00, 0x00, 0x00, 0x18, 0x66, 0x74, 0x79, 0x70}}},
    //     {"wav",  {"wav",  {0x52, 0x49, 0x46, 0x46}}},
    //     {"avi",  {"avi",  {0x52, 0x49, 0x46, 0x46}}},
    //     {"tar.gz", {"tar.gz", {0x1F, 0x8B, 0x08}}},
    //     {"conf", {"conf", {0x23, 0x21, 0x2F, 0x62, 0x69, 0x6E, 0x2F}}},
    //     {"txt",  {"txt",  {0x54, 0x45, 0x58, 0x54}}},
    //     {"sh",   {"sh",   {0x23, 0x21, 0x2F, 0x62, 0x69, 0x6E, 0x2F}}},
    //     {"xml",  {"xml",  {0x3C, 0x3F, 0x78, 0x6D, 0x6C}}},
    //     {"html", {"html", {0x3C, 0x21, 0x44, 0x4F, 0x43, 0x54, 0x59, 0x50, 0x45}}},
    //     {"csv",  {"csv",  {0x49, 0x44, 0x33}}} //often ID3 if they contain metadata
    // };

    static void filerecovery() {
        std::vector<std::string> drives;
        listDrives(drives);
        if (drives.empty()) {
            std::cout << "No dirves found\n";
            return;
        }
        std::cout << "\nEnter the name of a drive you want to scan for recoverable files:\n";
        std::string filerecoverydrivename;
        std::cin >> filerecoverydrivename;
        bool found = false;
        for (const auto& d : drives) {
            if (d == filerecoverydrivename) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::cout << "[Error] Invalid selection!\n";
            return;
        }
        enum signature {all = 0, png = 1, jpg = 2, elf = 3, zip = 4, pdf = 5, mp3 = 6, mp4 = 7, wav = 8, avi = 9, targz = 10, conf = 11, txt = 12, sh = 13, xml = 14, html = 15, csv = 16};
        enum scan_depth {quick = 1, full = 2};
        int sacn_detph_type;
        int signature_type;
        std::cout << "For what files do you want to search: ['all', 'png', 'jpg', 'elf', 'zip', 'pdf', 'mp3', 'mp4', 'wav', 'avi', 'tar.gz', 'conf', 'txt', 'sh', 'xml', 'html', 'csv']\n";
        std::string filerecovrysignatureinput;
        std::cin >> filerecovrysignatureinput;
        if (filerecovrysignatureinput != "all" && signatures.find(filerecovrysignatureinput) == signatures.end()) {
            std::cout << "[Error] Unsupported file type or invalid input\n";
            return;
        }
        switch (static_cast<signature>(signature_type)) {
            case all:
                signature_type = all;
                break;
            case png:
                signature_type = png;
                break;
            case jpg:
                signature_type = jpg;
                break;
            case elf:
                signature_type = elf;
                break;
            case zip:
                signature_type = zip;
                break;
            case pdf:
                signature_type = pdf;
                break;
            case mp3:
                signature_type = mp3;
                break;
            case mp4:
                signature_type = mp4;
                break;
            case wav:
                signature_type = wav;
                break;
            case avi:
                signature_type = avi;
                break;
            case targz:
                signature_type = targz;
                break;
            case conf:
                signature_type = conf;
                break;
            case txt:
                signature_type = txt;
                break;
            case sh:
                signature_type = sh;
                break;
            case xml:
                signature_type = xml;
                break;
            case html:
                signature_type = html;
                break;
            case csv:
                signature_type = csv;
                break;
            default:
                std::cout << "[Error] Invalid selection\n";
                return;  
        }
        int scan_depth_input;
        std::cout << "Wich Scan depth do you want [1 quick, 2 full]:\n";
        std::cin >> scan_depth_input;
        switch (static_cast<scan_depth>(scan_depth_input)) {
            case quick:
                
                break;
            case full:

                break;
            default:
                std::cerr << "[Error] invalid input or unexpected error\n";
        }

    }
    /*
    static void file_recovery_quick(const std::string& drive, int signature_type) {
        std::cout << "Scaning file for recoverable files (quick) signature type/s: " << signature_type << "...\n";
        // logic
        signatures[signature_type].quick_scan(drive);
        
    }
    */
    static void file_recovery_quick(const std::string& drive, int signature_type) {
        std::cout << "Scanning drive for recoverable files (quick) - signature index: " << signature_type << "...\n";

        // Mapping of the numeric menu choices to signature keys
        static const std::vector<std::string> signature_names = {
            "all", "png", "jpg", "elf", "zip", "pdf", "mp3", "mp4", "wav", "avi",
            "tar.gz", "conf", "txt", "sh", "xml", "html", "csv"
        };

        if (signature_type < 0 || static_cast<size_t>(signature_type) >= signature_names.size()) {
            std::cout << "[Error] Invalid signature type index.\n";
            return;
        }

        const std::string key = signature_names[signature_type];

        // Helper: scan a signature over the drive, limited to max_blocks (SIZE_MAX = full)
        auto scan_signature = [&](const file_signature& sig, size_t max_blocks) {
            if (sig.header.empty()) return;

            const size_t block_size = 4096;
            std::ifstream disk(drive, std::ios::binary);
            if (!disk.is_open()) {
                std::cerr << "[Error] Cannot open drive/image: " << drive << "\n";
                return;
            }

            std::vector<uint8_t> prev_tail;
            size_t offset = 0; // bytes read so far
            size_t blocks_read = 0;
            const size_t header_len = sig.header.size();

            while (disk && (max_blocks == SIZE_MAX || blocks_read < max_blocks)) {
                std::vector<char> buf(block_size);
                disk.read(buf.data(), block_size);
                std::streamsize n = disk.gcount();
                if (n <= 0) break;

                // window = prev_tail + buf
                std::vector<uint8_t> window;
                window.reserve(prev_tail.size() + static_cast<size_t>(n));
                window.insert(window.end(), prev_tail.begin(), prev_tail.end());
                window.insert(window.end(), reinterpret_cast<uint8_t*>(buf.data()), reinterpret_cast<uint8_t*>(buf.data()) + n);

                // search for header in window
                for (size_t i = 0; i + header_len <= window.size(); ++i) {
                    if (std::memcmp(window.data() + i, sig.header.data(), header_len) == 0) {
                        size_t found_offset = offset + i - prev_tail.size();
                        std::cout << "[FOUND] ." << sig.extension << " signature at offset: " << found_offset << "\n";
                    }
                }

                // keep the last (header_len - 1) bytes to handle signatures spanning blocks
                if (header_len > 1) {
                    size_t tail_len = std::min(window.size(), header_len - 1);
                    prev_tail.assign(window.end() - tail_len, window.end());
                } else {
                    prev_tail.clear();
                }

                offset += static_cast<size_t>(n);
                ++blocks_read;
            }

            disk.close();
        };

        if (key == "all") {
            // quick: limit to first N blocks per signature to stay fast
            const size_t quick_blocks = 1024; // ~4MB
            for (const auto& kv : signatures) {
                std::cout << "Quick scanning for: " << kv.first << "\n";
                scan_signature(kv.second, quick_blocks);
            }
        } else {
            auto it = signatures.find(key);
            if (it == signatures.end()) {
                std::cout << "[Error] Signature not found: " << key << "\n";
                return;
            }
            scan_signature(it->second, 1024);
        }
    }

    static void file_recovery_full(const std::string& drive, int signature_type) {
        std::cout << "Scanning drive for recoverable files (full) - signature index: " << signature_type << "...\n";

        static const std::vector<std::string> signature_names = {
            "all", "png", "jpg", "elf", "zip", "pdf", "mp3", "mp4", "wav", "avi",
            "tar.gz", "conf", "txt", "sh", "xml", "html", "csv"
        };

        if (signature_type < 0 || static_cast<size_t>(signature_type) >= signature_names.size()) {
            std::cout << "[Error] Invalid signature type index.\n";
            return;
        }

        const std::string key = signature_names[signature_type];

        auto scan_signature_full = [&](const file_signature& sig) {
            if (sig.header.empty()) return;
            const size_t block_size = 4096;
            std::ifstream disk(drive, std::ios::binary);
            if (!disk.is_open()) {
                std::cerr << "[Error] Cannot open drive/image: " << drive << "\n";
                return;
            }

            std::vector<uint8_t> prev_tail;
            size_t offset = 0;
            const size_t header_len = sig.header.size();

            while (disk) {
                std::vector<char> buf(block_size);
                disk.read(buf.data(), block_size);
                std::streamsize n = disk.gcount();
                if (n <= 0) break;

                std::vector<uint8_t> window;
                window.reserve(prev_tail.size() + static_cast<size_t>(n));
                window.insert(window.end(), prev_tail.begin(), prev_tail.end());
                window.insert(window.end(), reinterpret_cast<uint8_t*>(buf.data()), reinterpret_cast<uint8_t*>(buf.data()) + n);

                for (size_t i = 0; i + header_len <= window.size(); ++i) {
                    if (std::memcmp(window.data() + i, sig.header.data(), header_len) == 0) {
                        size_t found_offset = offset + i - prev_tail.size();
                        std::cout << "[FOUND] ." << sig.extension << " signature at offset: " << found_offset << "\n";
                    }
                }

                if (header_len > 1) {
                    size_t tail_len = std::min(window.size(), header_len - 1);
                    prev_tail.assign(window.end() - tail_len, window.end());
                } else {
                    prev_tail.clear();
                }

                offset += static_cast<size_t>(n);
            }

            disk.close();
        };

        if (key == "all") {
            for (const auto& kv : signatures) {
                std::cout << "Full scanning for: " << kv.first << "\n";
                scan_signature_full(kv.second);
            }
        } else {
            auto it = signatures.find(key);
            if (it == signatures.end()) {
                std::cout << "[Error] Signature not found: " << key << "\n";
                return;
            }
            scan_signature_full(it->second);
        }
    }

    static void partitionrecovery() {
        std::vector<std::string> drives;
        listDrives(drives);
        if (drives.empty()) {
            std::cout << "No drives found\n";
            return;
        }

        std::cout << "in development\n";
        std::cout << "\nEnter a name of a drive to try to recover partitions:\n";
        std::string Forenpartitionrecov;
        std::cin >> Forenpartitionrecov;

    }
    
    static void systemrecovery() {
        std::vector<std::string> drives;
        listDrives(drives);
        if (drives.empty()) {
            std::cout << "No drives found\n";
            return;
        }
        std::cout << "in development\n";
        std::cout << "\nEnter a name of a drive to try to recover the system on it:\n";
        std::string Forensysrecov;
        std::cin >> Forensysrecov;
    }
    //−·−
    // end of recovery
public:
    static void mainForensic(bool& running) {
        enum ForensicMenuOptions {
            Info = 1, CreateDisktImage = 2, ScanDrive = 3, Exit_Return = 0
        };
        std::cout << "\n-------- Forensic Analysis menu ---------\n";
        std::cout << "1. Info of the Analysis tool\n";
        std::cout << "2. Create a disk image of a drive\n";
        //std::cout << "3. recover of system, files,..\n";
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
            //case ScanDrive: {
                //recovery();
                //break;
            //}
                                                                                                                                                                                                                                                                                                     
            case Exit_Return:
                std::cout << "\nDo you want to return to the main menu or exit? (r/e)\n";
                char exitreturninput;
                std::cin >> exitreturninput;
                if (exitreturninput == 'r' || exitreturninput == 'R') return;
                else if (exitreturninput == 'e' || exitreturninput == 'E') {
                    running = false;
                    exit();
                }
                break;
                return;
            default:
                std::cout << "[Error] Invalid selection\n";
                return;
        }
    }
};



class DSV {
private:
    static long getSize(const std::string &path) {
        struct stat statbuf;
        if (stat(path.c_str(), &statbuf) == 0) {
            return statbuf.st_size;
        }
        return 0;
    }
    static void listFirstLayerFolders(const std::string &path) {
        DIR *dp = opendir(path.c_str());
        if (!dp) {
            std::cerr << "Error opening directory: " << path << '\n';
            return;
        }

        std::cout << "\n| " << std::setw(30) << std::left << "Name";
        std::cout << " | " << std::setw(10) << "Size";
        std::cout << " | Visualization\n";
        std::cout << std::string(60, '-') << std::endl;

        struct dirent *entry;
        while ((entry = readdir(dp)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    std::string fullPath = path + "/" + entry->d_name;
                    long size = getSize(fullPath);
                    std::cout << "| " << std::setw(30) << std::left << entry->d_name;
                    std::cout << " | " << std::setw(10) << size / (1024 * 1024) << " MB |";
                    //std::cout << std::string(20, '') << "\n";
                }
            }
        }
        closedir(dp);
    }

public:
    static void DSVmain() {
        std::vector<std::string> drives;
        listDrives(drives);
        std::cout << "\nDisk space visulizer\n";
        if (drives.empty()) {
            std::cout << "[Error] No drives found!\n";
            return;
        }

        std::cout << "Select a number of a drive to visualize its contents: ";
        int DSVdriveChoice;
        std::cin >> DSVdriveChoice;
        if (DSVdriveChoice < 0 || DSVdriveChoice >= drives.size()) {
            std::cout << "Invalid selection.\n";
            return;
        }
        std::string currentPath = "/"; 
        listFirstLayerFolders(currentPath);
        while (true) {
            std::cout << "Enter folder to explore, '..' to go up, or 'exit' to quit: ";
            std::string input;
            std::cin >> input;

            if (input == "exit") {
                break;
            } else if (input == "..") {
                size_t pos = currentPath.find_last_of('/');
                if (pos != std::string::npos) {
                    currentPath = currentPath.substr(0, pos);
                }
            } else {
                std::string nextPath = currentPath + input;
                if (getSize(nextPath) > 0) {
                    currentPath = nextPath;
                    listFirstLayerFolders(currentPath);
                } else {
                    std::cout << "Directory does not exist or cannot be accessed.\n";
                }
            }
        }
    }
};


void log_viewer() {
    const char* sudo_user = getenv("SUDO_USER");
    const char* user_env = getenv("USER");
    const char* username = sudo_user ? sudo_user : user_env;

    if (!username) {
        std::cerr << "[Error] Could not determine username.\n";
        return;
    }
    struct passwd* pw = getpwnam(username);
    if (!pw) {
        std::cerr << "[Error] Could not get home directory for user: " << username << "\n";
        return;
    }
    std::string homeDir = pw->pw_dir;
    std::string path = homeDir + "/.var/app/DriveMgr/log.dat";
    std::ifstream file(path);
    if (!file) {
        Logger::log("[ERROR] Unable to read log file at " + path);
        std::cerr << "[Error] Unable to read log file at path: " << path << "\n";
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << "\n";
    }
}


// main and Info
void Info() {
    std::cout << "\n┌────────── Info ──────────\n";
    std::cout << "| Welcome to Drive Manager, this is a program for linux to view, operate,... your Drives in your system\n";
    std::cout << "| Warning! You should know some basic things about drives so you dont loose any data\n";
    std::cout << "| If you find any problems/issues or have ideas, visit my Github page and send message\n";
    std::cout << "| Other info:\n";
    std::cout << "| Version: 0.8.90\n";
    std::cout << "| Github: https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux\n";
    std::cout << "| Author: Dogwalker-kryt\n";
    std::cout << "└───────────────────────────\n";
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
    CHECKDRIVEHEALTH = 5, ANALYZEDISKSPACE = 6, OVERWRITEDRIVEDATA = 7, VIEWMETADATA = 8, VIEWINFO = 9,
    MOUNTUNMOUNT = 10, FORENSIC = 11, DISKSPACEVIRTULIZER = 12, FUNCTION999 = 999, LOGVIEW = 13
};

int main() {
    bool running = true;
    while (running == true) {
        std::string clear = Terminalexec::execTerminal("clear");
        std::cout << clear;
        std::cout << "┌─────────────────────────────────────────────────┐\n";
        std::cout << "│              DRIVE MANAGEMENT UTILITY           │\n";
        std::cout << "├─────────────────────────────────────────────────┤\n";
        std::cout << "│ 1.  List Drives                                 │\n";
        std::cout << "│ 2.  Format Drive                                │\n";
        std::cout << "│ 3.  Encrypt/Decrypt Drive (AES-256)             │\n";
        std::cout << "│ 4.  Resize Drive                                │\n";
        std::cout << "│ 5.  Check Drive Health                          │\n";
        std::cout << "│ 6.  Analyze Disk Space                          │\n";
        std::cout << "│ 7.  Overwrite Drive Data                        │\n";
        std::cout << "│ 8.  View Drive Metadata                         │\n";
        std::cout << "│ 9.  View Info                                   │\n";
        std::cout << "│10.  Mount/Unmount/restore (ISO/Drives/USB)      │\n";
        std::cout << "│11.  Forensic Analysis (Beta)                    │\n";
        std::cout << "│12.  Disk Space Visualizer (Beta)                │\n";
        std::cout << "│13.  Log viewer                                  │\n";
        std::cout << "│ 0.  Exit                                        │\n";
        std::cout << "└─────────────────────────────────────────────────┘\n";
        std::cout << "choose an option [0 - 12]:\n";
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
            case FUNCTION999:
                break;
            case MOUNTUNMOUNT:
                MountUtility::mainMountUtil();
                MenuQues(running);
                break;
            case FORENSIC:
                ForensicAnalysis::mainForensic(running);
                break;
            case DISKSPACEVIRTULIZER:
                DSV::DSVmain();
                MenuQues(running);
                break; 
            case EXITPROGRAM:
                running = false;
                break;
            case LOGVIEW:
                log_viewer();
                MenuQues(running);
                break;
            default:
                std::cout << "[Error] Invalid selection\n";
                break;
        }
    }
    return 0;
}
