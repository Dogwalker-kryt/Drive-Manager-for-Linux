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

// ! Warning this version is the experimental version of the program,
// This version has the latest and newest functions, but may contain bugs and errors
// Current version of this code is in the Info() function below
// v0.8.99-35-experimental
//
// standard C++ libraries, I think
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
#include <dirent.h>
#include <map>
#include <cstdint>
#include <fcntl.h>
#include <libgen.h>
#include <ctime>
#include <random>
// system
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
// openssl
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
// custom .h
#include "../include/drivefunctions.h"

//helping/side funtion

// Stores the last drives printed by ListDrives()
static std::vector<std::string> g_last_drives;

// Forward declaration so checkDriveName() can call it safely
void ListDrives();

void checkDriveName(const std::string &driveName) {
    // Ensure the global list is fresh and printed (uses ListDrives as requested)
    ListDrives();

    bool drive_found = false;
    for (const auto& drive : g_last_drives) {
        // match either full device path (/dev/sda) or the basename (sda)
        if (drive == driveName || std::filesystem::path(drive).filename() == std::filesystem::path(driveName).filename()) {
            drive_found = true;
            break;
        }
    }

    if (!drive_found) {
        std::cerr << "[Error] Drive '" << driveName << "' not found!\n";
        Logger::log("[ERROR] Drive not found");
        return;
    }
}

std::string confirmationKeyGenerator() {
    const std::string chars =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";
    std::string displayKey;
    std::mt19937 gen(std::time(0));
    std::uniform_int_distribution<> dist(0, chars.size() - 1);

    for (int i = 0; i < 10; i++) {
        displayKey += chars[dist(gen)];
    }
    return displayKey;
}

bool askForConfirmation(const std::string &prompt) {
    std::cout << prompt << "(y/n)\n";
    char confirm;
    std::cin >> confirm;
    if (confirm != 'Y' && confirm != 'y') {
        std::cout << "[INFO] Operation cancelled\n";
        Logger::log("[INFO] Operation cancelled");
        return false;
    }
    return true;
}

std::string getAndValidateDriveName(const std::string& prompt) {
    ListDrives();

    if (g_last_drives.empty()) {
        std::cerr << "[Error] No drives available to select!\n";
        Logger::log("[ERROR] No drives available to select");
        return "";
    }

    std::cout << "\n" << prompt << ":\n";
    std::string driveName;
    std::cin >> driveName; 
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
    
    if (driveName.empty()) {
        std::cerr << "[Error] Drive name cannot be empty!\n";
        Logger::log("[ERROR] drive name cannot be empty\n");
        return "";
    }

    if (driveName.find_first_of("--'&|<>;\"") != std::string::npos) {
        std::cerr << "[Error] Invalid characters in drive name!\n";
        Logger::log("[ERROR] Invalid characters in drive name\n");
        return "";  
    }


    if (driveName.find("/dev/") != 0) {
        std::cerr << "[Error] Invalid drive name or no drives available!\n";
        Logger::log("[ERROR] Invalid drive name or no drives available");
        return "";
    }

    bool drive_found = false;
    for (const auto& drive : g_last_drives) {
        // match either full device path (/dev/sda) or the basename (sda)
        if (drive == driveName || std::filesystem::path(drive).filename() == std::filesystem::path(driveName).filename()) {
            drive_found = true;
            break;
        }
    }

    if (!drive_found) {
        std::cerr << "[Error] Drive '" << driveName << "' not found!\n";
        Logger::log("[ERROR] Drive not found");
        return "";
    }

    return driveName;
}


void file_recovery_quick(const std::string& drive, const std::string& signature_key);
void file_recovery_full(const std::string& drive, const std::string& signature_key);

//main functions

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

void ListDrives();

// ListDrives
void listDrives(std::vector<std::string>& drives) {
    drives.clear();
    std::string lsblk = Terminalexec::execTerminal("lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -d -n -p");
    std::cout << "\nAvilable Drives:\n";
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


void ListDrives() {
    std::vector<std::string> drives;
    listDrives(drives);

    if (drives.empty()) {
        std::cerr << "No drives found!\n";
        Logger::log("[ERROR] No Drives found!");
        return;
    }

    // Cache the last-listed drives so checkDriveName() can use them
    g_last_drives = drives;
}


// Partitions
class PartitionsUtils {
    public:
        // 1
        static bool resizePartition(const std::string& device, uint64_t newSizeMB) {
            try {
                std::string resize_partition_cmd = "parted --script " + device + " resizepart 1 " + 
                                 std::to_string(newSizeMB) + "MB";
                std::string resize_partition_cmd_output = Terminalexec::execTerminal(resize_partition_cmd.c_str());
                return resize_partition_cmd_output.find("error") == std::string::npos;
            } catch (const std::exception&) {
                return false;
            }
        }
        // 2
        static bool movePartition(const std::string& device, int partNum, uint64_t startSectorMB) {
            try {
                std::string move_partition_cmd = "parted --script " + device + " move " + 
                                 std::to_string(partNum) + " " + 
                                 std::to_string(startSectorMB) + "MB";
                std::string move_partition_cmd_output = Terminalexec::execTerminal(move_partition_cmd.c_str());
                return move_partition_cmd_output.find("error") == std::string::npos;
            } catch (const std::exception&) {
                return false;
            }
        }
        // 3
        static bool changePartitionType(const std::string& device, int partNum, const std::string& newType) {
            try {
                std::string backupCmd = "sfdisk -d " + device + " > " + device + "_backup.sf";
                Terminalexec::execTerminal(backupCmd.c_str());

                std::string change_partition_cmd = "echo 'type=" + newType + "' | sfdisk --part-type " + 
                                 device + " " + std::to_string(partNum);
                std::string change_partition_cmd_output = Terminalexec::execTerminal(change_partition_cmd.c_str());
                return change_partition_cmd_output.find("error") == std::string::npos;
            } catch (const std::exception&) {
                return false;
            }
        }
};

void listpartisions(std::vector<std::string>& drive) {
    std::string drive_name = getAndValidateDriveName("Enter the Name of the Drive you want to see the partitions of");

    std::cout << "\nPartitions of drive " << drive_name << ":\n";
    std::string list_partitions_cmd = "lsblk --ascii -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -n -p " + drive_name;
    std::string list_partitions_cmd_output = Terminalexec::execTerminal(list_partitions_cmd.c_str());
    std::istringstream iss(list_partitions_cmd_output);
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
            std::string part_name;
            lss >> part_name;
            partitions.push_back(part_name);
        }
    }

    if (partitions.empty()) {
        std::cout << "No partitions found on this drive.\n";
        return;
    }

    std::cout << "\nPartition Management Options:\n";
    std::cout << "-------------------------------\n";
    std::cout << "1. Resize partition\n";
    std::cout << "2. Move partition\n";
    std::cout << "3. Change partition type\n";
    std::cout << "4. Return to main menu\n";
    std::cout << "------------------------------\n";
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

            askForConfirmation("[Warning] Resizing partitions can lead to data loss.\nAre you sure? ");

            if (PartitionsUtils::resizePartition(partitions[partNum-1], newSize)) {
                std::cout << "Partition resized successfully!\n";
            } else {
                std::cout << "Failed to resize partition!\n";
                Logger::log("[ERROR] Failed to resize partition");
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
            if (startPos < 0) {
                std::cout << "Invalid start position!\n";
                break;
            }

            askForConfirmation("[Warning] Moving partitions can lead to data loss.\nAre you sure? ");

            if (PartitionsUtils::movePartition(partitions[partNum-1], partNum, startPos)) {
                std::cout << "Partition moved successfully!\n";
            } else {
                std::cout << "Failed to move partition!\n";
                Logger::log("[ERROR] Failed to move partition");
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
            std::cout << "--------------------------\n";
            std::cout << "1. Linux (83)\n";
            std::cout << "2. NTFS (7)\n";
            std::cout << "3. FAT32 (b)\n";
            std::cout << "4. Linux swap (82)\n";
            std::cout << "--------------------------\n";
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
                askForConfirmation("[Warning] Changing partition type can make data inaccessible.\nAre you sure? ");
                if (PartitionsUtils::changePartitionType(drive_name, partNum, newType)) {
                    std::cout << "Partition type changed successfully!\n";
                } else {
                    std::cout << "Failed to change partition type!\n";
                    Logger::log("[ERROR] Failed to change partition type");
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
    std::string drive_name = getAndValidateDriveName("Enter the Name of the Drive you want to analyze");

    std::cout << "\n------ Disk Information ------\n";
    std::string analy_disk_space_cmd = "lsblk -b -o NAME,SIZE,TYPE,MOUNTPOINT -n -p " + drive_name;
    std::string analy_disk_space_cmd_output = Terminalexec::execTerminal(analy_disk_space_cmd.c_str());
    std::istringstream iss(analy_disk_space_cmd_output);
    std::string line;
    bool found = false;
    std::string mount_point;
    std::string size;

    while (std::getline(iss, line)) {
        std::istringstream lss(line);
        std::string name, type;
        lss >> name >> size >> type;
        std::getline(lss, mount_point);
        if (!mount_point.empty() && mount_point[0] == ' ') mount_point = mount_point.substr(1);

        if (type == "disk") {
            found = true;
            std::cout << "Device:      " << name << "\n";
            try {
                unsigned long long bytes = std::stoull(size);
                const char* units[] = {"B", "KB", "MB", "GB", "TB"};
                int unit = 0;
                double human_size = bytes;
                while (human_size >= 1024 && unit < 4) {
                    human_size /= 1024;
                    ++unit;
                }
                std::cout << "Size:        " << human_size << " " << units[unit] << "\n";
            } catch (...) {
                std::cout << "Size:        " << size << " bytes\n";
            }
            std::cout << "Type:        " << type << "\n";
            std::cout << "Mountpoint:  " << (mount_point.empty() ? "-" : mount_point) << "\n";
        }
    }
    if (!found) {
        std::cout << "No disk info found!\n";
    } else {
        if (!mount_point.empty() && mount_point != "-") {
            std::string df_cmd = "df -h '" + mount_point + "' | tail -1";
            std::string df_out = Terminalexec::execTerminal(df_cmd.c_str());
            std::istringstream dfiss(df_out);
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
    std::cout << "------------------\n";
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
                std::string driveName = getAndValidateDriveName("Enter name of a drive you want to format");

                std::cout << "Are you sure you want to format drive " << driveName << "? (y/n):\n";
                std::string confirmationfd;
                std::cin >> confirmationfd;
                if (confirmationfd == "y" || confirmationfd == "Y" ) {
                    std::cout << "Formatting drive: " << driveName << "...\n";
                    std::string cmd = "mkfs.ext4 " + driveName;
                    std::string result = Terminalexec::execTerminal(cmd.c_str());
                    if (result.find("error") != std::string::npos) {
                        Logger::log("[ERROR] Failed to format drive: " + driveName);
                        std::cout << "[Error] Failed to format drive: " << driveName << "\n";
                    } else {
                        Logger::log("[INFO] Drive formatted successfully: " + driveName);
                        std::cout << "Drive formatted successfully: " << driveName << "\n";
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
                std::string driveName = getAndValidateDriveName("Enter name of a drive you want to format with label");

                std::cout << "Enter label: ";
                std::string label;
                std::cin >> label;
                std::cout << "Formatting drive with label: " << label << "\n";
                if (label.empty()) {
                    std::cout << "[Error] label cannot be empty!\n";
                    Logger::log("[ERROR] label cannot be empty");
                    return;
                }

                std::cout << "Are you sure you want to format drive " << driveName << " with label '" << label << "' ? (y/n)\n";
                char confirmationfd;
                std::cin >> confirmationfd;
                if (confirmationfd != 'y' && confirmationfd != 'Y') {
                    std::cout << "[Info] Formating canclled!\n";
                    Logger::log("[INFO] formatting cancelled");
                    break;
                }

                std::string execTerminal(("mkfs.ext4 -L " + label + " " + driveName).c_str());
                if (execTerminal.find("error") != std::string::npos) {
                    std::cout << "[Error] Failed to format drive: " << driveName << "\n";
                    Logger::log("[ERROR] Failed to format drive: " + driveName);
                } else {
                    std::cout << "[INFO] Drive formatted successfully with label: " << label << "\n";
                    Logger::log("[INFO] Drive formatted successfully with label: " + label + " -> foramtDrvie()");
                }
            }
            break;
        case 3:
            {
                std::string driveName = getAndValidateDriveName("Enter name of a drive you want to format with label and filesystem type");

                std::string label, fsType;
                std::cout << "Enter label: ";
                std::cin >> label;
                std::cout << "Enter filesystem type (e.g. ext4): ";
                std::cin >> fsType;
                std::cout << "Are you sure you want to format drive " << driveName << " with label '" << label << "' and filesystem type '" << fsType << "' ? (y/n)\n";
                char confirmation_fd3;
                std::cin >> confirmation_fd3;
                if (confirmation_fd3 != 'y' && confirmation_fd3 != 'Y') {
                    std::cout << "[Info] Formating cancelled!\n";
                    Logger::log("[INFO] Format operation cancelle by user -> formatDrive()");
                    return;
                }

                std::string execTerminal(("mkfs." + fsType + " -L " + label + " " + driveName.c_str()));
                if (execTerminal.find("error") != std::string::npos) {
                    std::cout << "[Error] Failed to format drive: " << driveName << "\n";
                    Logger::log("[ERROR] Failed to format drive: " + driveName);
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
    std::string driveHealth_name = getAndValidateDriveName("Enter the name of the drive to check health");

    try {
        std::string check_drive_helth_cmd = "sudo smartctl -H " + driveHealth_name;
        std::string check_drive_helth_cmd_output = Terminalexec::execTerminal(check_drive_helth_cmd.c_str());
        std::cout << check_drive_helth_cmd_output;
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
    std::string driveName = getAndValidateDriveName("Enter the name of the drive to resize");

    std::cout << "Enter new size in GB for drive " << driveName << ":\n";
    int new_size;
    std::cin >> new_size;
    if (new_size <= 0) {
        std::cout << "[Error] Invalid size entered!\n";
        return;
    }

    std::cout << "Resizing drive " << driveName << " to " << new_size << " GB...\n";

    if (resizeDrive(driveName, new_size)) {
        std::cout << "Drive resized successfully\n";
    } else {
        std::cout << "[Error] Failed to resize drive\n";
        Logger::log("[ERROR] Failed to resize drive: " + driveName + " -> resizeDrive()");
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
                << "open " << driveName << " encrypted_" << std::filesystem::path(driveName).filename().string();
            Logger::log("[INFO] Encrypting drive: " + driveName);
            std::string output = Terminalexec::execTerminal(ss.str().c_str());

            if (output.find("Command failed") != std::string::npos) {
                std::cerr << "[Error] Encryption failed: " << output << "\n";
                Logger::log("[ERROR] Encryption failed for drive: " + driveName + " -> encryptDrive()");
                return;
            }
            std::cout << "Drive encrypted successfully. The decryption key is stored in " << KEY_STORAGE_PATH << "\n";
            Logger::log("[INFO] Drive encrypted successsfully: " + driveName + " -> encryptDrive()");
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
                << "open " << driveName << " decrypted_" << std::filesystem::path(driveName).filename().string();
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
        static std::vector<unsigned char> generateSalt(size_t length = 16)  {
            std::vector<unsigned char> salt(length);
            if (!RAND_bytes(salt.data(), length)) {
                Logger::log("[ERROR] Failed to generate salt");
                throw std::runtime_error("[Error] Failed to generate salt");
            }
            return salt;
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

class DeEncrypting {
private:
    static void encrypting() {
        std::string driveName = getAndValidateDriveName("Enter the NAME of a drive to encrypt:\n");

        std::cout << "[Warning] Are you sure you want to encrypt " << driveName << "? (y/n)\n";
        char endecrypt_confirm;
        std::cin >> endecrypt_confirm;
        if (endecrypt_confirm != 'y' && endecrypt_confirm != 'Y') {
            std::cout << "[Info] Encryption cancelled.\n";
            Logger::log("[INFO] Encryption cancelled -> void EnDecrypt()");
            return;
        }

        std::string Key = confirmationKeyGenerator();

        std::cout << "\nPlease enter the confirmationkey to proceed with the operation:\n";
        std::cout << Key << "\n";
        std::string random_confirmationkey_input3;
        std::cin >> random_confirmationkey_input3;
        if (random_confirmationkey_input3 != Key) {
            std::cout << "[Error] Invalid confirmation of the Key or unexpected error\n";
            Logger::log("[ERROR] Invalid confirmation of the Key or unexpected error -> EnDecrypt");
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
        std::string device_Name_Encrypt;
        std::cin >> device_Name_Encrypt;
        
        std::stringstream ss;
        ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
           << "--key-file <(echo -n '" << std::string((char*)info.key, 32) << "') "
           << "open " << driveName << " " << device_Name_Encrypt;
        
        std::string output = Terminalexec::execTerminal(ss.str().c_str());
        if (output.find("Command failed") != std::string::npos) {
            std::cout << "[Error] Failed to encrypt the drive: " << output << "\n";
            Logger::log("[ERROR] failed to encrypt the drive -> void EnDecrypt()");
        } else {
            std::cout << "[Success] Drive " << driveName << " has been encrypted as " << device_Name_Encrypt << "\n";
            std::cout << "[Info] The decryption key is stored in " << KEY_STORAGE_PATH << "\n";
            Logger::log("[INFO] Key saved -> void EnDecrypt()");
        }
    }

    static void decrypting() {
        std::string driveName = getAndValidateDriveName("Enter the NAME of a drive to decrypt:\n");

        std::cout << "[Warning] Are you sure you want to decrypt " << driveName << "? (y/n)\n";
        char decryptconfirm;
        std::cin >> decryptconfirm;
        if (decryptconfirm != 'y' && decryptconfirm != 'Y') {
            std::cout << "[Info] Decryption cancelled.\n";
            Logger::log("[INFO] Ddecryption cancelled -> void EnDecrypt()");
            return;
        }
        
        std::string Key = confirmationKeyGenerator();

        std::cout << "\nPlease enter the confirmationkey to proceed with the operation:\n";
        std::cout << Key << "\n";
        std::string random_confirmationkey_input2;
        std::cin >> random_confirmationkey_input2;
        if (random_confirmationkey_input2 != Key) {
            std::cout << "[Error] Invalid confirmation of the Key or unexpected error\n";
            Logger::log("[ERROR] Invalid confirmation of the Key or unexpected error -> EnDecryptDrive");
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

    }

public:
    static void main() {
        std::cout << "Do you want to 'e'ncrypt or 'd'ecrypt?:\n";
        char DeEncryption_main_input;
        std::cin >> DeEncryption_main_input;

        if (DeEncryption_main_input == 'e') {
            DeEncrypting::encrypting();
        } else if (DeEncryption_main_input == 'd') {
            DeEncrypting::decrypting();
        } else {
            std::cerr << "[ERROR] Wrong input or unexpected error occurred\n";
        }
    }
};


// Overwrite drive data
void OverwriteDriveData() {
    std::string driveName = getAndValidateDriveName("Enter the NAME of a drive to overwrite all data (this will erase everything):\n");

    std::cout << "Are you sure you want to overwrite the drive " << driveName << "? (y/n)\n";
    char confirmation_zero_drive;
    std::cin >> confirmation_zero_drive;
    
    if (confirmation_zero_drive == 'y' || confirmation_zero_drive == 'Y' ) {
        // srand(static_cast<unsigned int>(time(0))); 
        // char random_confirmation_key[] = {'a', 'A', 'b', 'B', 'c', 'C', 'd', 'D', 'e', 'E', 'f', 'F', 'g', 'G', 'h', 'H', 'i', 'I', 'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N', 'o', 'O', 'p', 'P', 'q', 'Q', 'r', 'R', 's', 'S', 't', 'T', 'u', 'U', 'v', 'V', 'w', 'W', 'x', 'X', 'y', 'Y', 'z', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        // std::string displayKey;
        // for (int i = 0; i < 10; i++) {
        //     int randomIndex = rand() % (sizeof(random_confirmation_key) / sizeof(random_confirmation_key[0]));
        //     displayKey += random_confirmation_key[randomIndex];
        // }
        std::string Key = confirmationKeyGenerator();

        std::cout << "\nPlease enter the confirmationkey to proceed with the operation:\n";
        std::cout << Key << "\n";
        std::string random_confirmation_key_input;
        std::cin >> random_confirmation_key_input;
        if (random_confirmation_key_input != Key) {
            std::cout << "[Error] Invalid confirmation of the Key or unexpected error\n";
            Logger::log("[ERROR] Invalid confirmation of the Key or unexpected error -> OverwriteData");
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
                std::cout << "[Warning] failed to Overwrite, only one of two overwriting opeartions succeeded\n"
                          << "Try again";
                Logger::log("[WARNING] failed to overwrite, only one overwriting operation succeeded");
                return;

            } else if (devZero.find("failed") != std::string::npos || devrandom.find("failed") != std::string::npos) {
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
            Logger::log("[ERROR] Overwrite failed for drive: " + driveName + " Reason: " + e.what());
            throw std::runtime_error("[Error] Overwrite failed: " + std::string(e.what()));
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
                std::cout << "[ERROR] SMART data not available/intalled\n";
            }
        }                                           
        std::cout << "└─  - -─ --- ─ - -─-  - ──- ──- ───────────────────\n";
                    
    }
    
public:
    static void mainReader() {
        // ListDrives();
        // std::cout << "\nEnter Drive Name for reading metadata\n";
        // std::string driveName;
        // std::cin >> driveName;
        // checkDriveName(driveName);
        std::string driveName = getAndValidateDriveName("Enter Drive name for reding metadata");


        try {
            DriveMetadata metadata = getMetadata(driveName);
            displayMetadata(metadata);
            Logger::log("[INFO] Successfully read metadata for drive: " + driveName);
        } catch (const std::exception& e) {
            std::cout << "[Error] Failed to read drive metadata: " << e.what() << "\n";
            Logger::log("[ERROR] Failed to read drive metadata: " + std::string(e.what()));
        }
    }
};


class MountUtility {
private:
    static void BurnISOToStorageDevice() { //const std::string& isoPath, const std::string& drive
        std::string driveName = getAndValidateDriveName("[Burn ISO/IMG] Enter the Name of a drive you want to burn an iso/img to:");

        std::cout << "Are you sure you want to select " << driveName << " for this operation? (y/n)\n";
        char confirmation_burn;
        std::cin >> confirmation_burn;
        if (confirmation_burn != 'y' && confirmation_burn != 'Y') {
            std::cout << "[Info] Opeartion cancelled\n";
            Logger::log("[INFO] Operation cancelled -> BurnISOToStorageDevice()");
            return;
        }

        std::cout << "\nEnter the path to the iso/img file you want to burn on " << driveName << ":\n";
        std::string isoPath;
        std::cin >> isoPath;
        std::cout << "Are you sure you want to burn " << isoPath << " to " << driveName << "? (y/n)\n";
        char confirmation_burn2;
        std::cin >> confirmation_burn2;
        if (confirmation_burn2 != 'y' && confirmation_burn2 != 'Y') {
            std::cout << "[Info] Operation cancelled\n";
            Logger::log("[INFO] Operation cancelled -> BurnISOToSotorageDevice()");
            return;
        }

        char random_confirmationkey[] = {'a', 'A', 'b', 'B', 'c', 'C', 'd', 'D', 'e', 'E', 'f', 'F', 'g', 'G', 'h', 'H', 'i', 'I', 'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N', 'o', 'O', 'p', 'P', 'q', 'Q', 'r', 'R', 's', 'S', 't', 'T', 'u', 'U', 'v', 'V', 'w', 'W', 'x', 'X', 'y', 'Y', 'z', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        for (int i = 0; i < 10; i++) {
            int randomIndex = rand() % (sizeof(random_confirmationkey) / sizeof(random_confirmationkey[0]));
            std::cout << random_confirmationkey[randomIndex];
        }

        std::cout << "\nPlease enter the confirmationkey to proceed with the operation:\n";
        std::cout << random_confirmationkey << "\n";
        char random_confirmationkey_input[10];
        std::cin >> random_confirmationkey_input;
        if (std::string(random_confirmationkey_input) != std::string(random_confirmationkey)) {
            std::cout << "[Error] Invalid confirmation of the Key or unexpected error\n";
            Logger::log("[ERROR] Invalid confirmation of the Key or unexpected error -> OverwriteData");
            return;
        } else {
            try {
                std::cout << "Proceeding with burning " << isoPath << " to " << driveName << "...\n";
                std::string bruncmd = "sudo dd if=" + isoPath + " of=" + driveName + " bs=4M status=progress && sync";
                std::string brunoutput = Terminalexec::execTerminalv2(bruncmd.c_str());
                if (brunoutput.find("error") != std::string::npos) {
                    Logger::log("[ERROR] Failed to burn iso/img to drive: " + driveName + " -> BurnISOToStorageDevice()"); 
                    throw std::runtime_error("[Error] Faile to burn iso/img to drive: " + driveName);
                }

                std::cout << "[Success] Successfully burned " << isoPath << " to " << driveName << "\n";
                Logger::log("[INFO] Successfully burned iso/img to drive: " + driveName + " -> BurnISOToStorageDevice()");
            } catch (const std::exception& e) {
                std::cout << e.what() << "\n";
            } 
        }
    }

    static void MountDrive2() {
        std::string driveName = getAndValidateDriveName("[Mount] Enter the Name of a drive you want to mount:");

    std::string mountpoint = "sudo mount " + driveName + " /mnt/" + std::filesystem::path(driveName).filename().string();
        std::string mountoutput = Terminalexec::execTerminalv2(mountpoint.c_str());
        if (mountoutput.find("error") != std::string::npos) {
            std::cout << "[Error] Failed to mount drive: " << mountoutput << "\n";
            Logger::log("[ERROR] Failed to mount drive: " + driveName + " -> MountDrive()");
            return;
        }
    }

    static void UnmountDrive2() {
        std::string driveName = getAndValidateDriveName("[Unmount] Enter the Name of a drive you want to unmount:");

        std::string unmountpoint = "sudo umount " + driveName;
        std::string unmountoutput = Terminalexec::execTerminalv2(unmountpoint.c_str());
        if (unmountoutput.find("error") != std::string::npos) {
            std::cout << "[Error] Failed to unmount drive: " << unmountoutput << "\n";
            Logger::log("[ERROR] Failed to unmount drive: " + driveName + " -> UnmountDrive()");
            return;
        }
    }

    static void Restore_USB_Drive() {
        std::string restore_device_name = getAndValidateDriveName("[Restore] Enter the name of a USB device you want to restore from an ISO");
        // if (std::find(drives.begin(), drives.end(), restore_device_name) == drives.end()) {
        //     std::cout << "[Error] Drive " << restore_device_name << " not found!\n";
        //     Logger::log("[ERROR] Drive " + restore_device_name + " not found -> Restore_USB_Drive()");
        //     return;
        // }

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
                Restore_USB_Drive();
                break;
            }
            case Exit: {
                return;
            }
            default: {
                std::cout << "[Error] Invalid selection\n";
                return;
            }
        }
    }
};


class ForensicAnalysis {
private:
    static int exit() {
        return 0;
    }

    static void CreateDiskImage() {
        std::string driveName = getAndValidateDriveName("[Drive_image_creation] Enter the NAME of a drive to create a disk image (e.g., /dev/sda):");

        std::cout << "Enter the path where the disk image should be saved (e.g., /path/to/image.img):\n";
        std::string imagePath;
        std::cin >> imagePath;
        std::cout << "Are you sure you want to create a disk image of " << driveName << " at " << imagePath << "? (y/n)\n";
        char confirmationcreate;
        std::cin >> confirmationcreate;
        if (confirmationcreate != 'y' && confirmationcreate != 'Y') {
            std::cout << "[Info] Operation cancelled\n";
            Logger::log("[INFO] Operation cancelled -> CreateDiskImage()");
            return;
        }

        std::string cmd = "sudo dd if=" + driveName + " of=" + imagePath + " bs=4M status=progress && sync";
        std::string output = Terminalexec::execTerminalv2(cmd.c_str());

        if (output.find("error") != std::string::npos) {
            std::cout << "[Error] Failed to create disk image: " << output << "\n";
            Logger::log("[ERROR] Failed to create disk image for drive: " + driveName + " -> CreateDiskImage()");
            return;
        }
        std::cout << "[Success] Disk image created at " << imagePath << "\n";
        Logger::log("[INFO] Disk image created successfully for drive: " + driveName + " -> CreateDiskImage()");
    }

    // recoverymain + side functions
    static void recovery() {
        std::cout << "\n-------- Recovery ---------−−−\n";
        std::cout << "1. files recovery\n";
        std::cout << "2. partition recovery\n";
        std::cout << "3. system recovery\n";
        std::cout << "--------------------------------\n";
        std::cout << "In development...\n";
        int scanDriverecover;
        std::cin >> scanDriverecover;
        switch (scanDriverecover) {
            case 1: {
                filerecovery();
                break;
            }
            case 2: {
                partitionrecovery();
                break;
            }
            case 3: {
                systemrecovery();
                break;
            }
            default: {
                std::cout << "[Error] invalid input\n";  
                break;     
            }  
        }   
    }

    //·−−− recovery side functions
    static void filerecovery() {
        std::string device = getAndValidateDriveName("Enter the NAME of a drive or image to scan for recoverable files (e.g., /dev/sda:");

        static const std::vector<std::string> signature_names = {"all","png","jpg","elf","zip","pdf","mp3","mp4","wav","avi","tar.gz","conf","txt","sh","xml","html","csv"};
        std::cout << "Type signature to search (e.g. png) or 'all':\n";
        std::string sig_in;
        std::cin >> sig_in;
        size_t sig_idx = SIZE_MAX;

        for (size_t i = 0; i < signature_names.size(); ++i) if (signature_names[i] == sig_in) { sig_idx = i; break; }
        if (sig_idx == SIZE_MAX) { std::cout << "[Error] Unsupported signature\n"; return; }

        std::cout << "Scan depth: 1=quick 2=full\n";
        int depth = 0;
        std::cin >> depth;

        if (depth == 1) file_recovery_quick(device, (int)sig_idx);
        else if (depth == 2) file_recovery_full(device, (int)sig_idx);
        else std::cout << "[Error] Invalid depth\n";
    }

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
        // ListDrives();
        // std::cout << "\nEnter the name of the drive to analyse for partition recovery (e.g. /dev/sdb):\n";
        // std::string device;
        // std::cin >> device;
        // checkDriveName(device);
        std::string device = getAndValidateDriveName("Enter the name of the drive to analyse for partition recovery (e.g. /dev/sdb):");

        std::cout << "\n--- Partition table (parted) ---\n";
        std::string parted_out = Terminalexec::execTerminalv2(("sudo parted -s " + device + " print").c_str());
        std::cout << parted_out << "\n";

        std::cout << "\n--- fdisk -l ---\n";
        std::string fdisk_out = Terminalexec::execTerminalv2(("sudo fdisk -l " + device + " 2>/dev/null || true").c_str());
        std::cout << fdisk_out << "\n";

        // Offer to dump partition table using sfdisk (non-destructive)
        std::cout << "Would you like to save a partition-table dump (recommended) to a file for possible restoration? (y/N): ";
        char saveDump = 'n';
        std::cin >> saveDump;
        std::string dumpPath;

        if (saveDump == 'y' || saveDump == 'Y') {
            dumpPath = device;

            // sanitize filename: replace '/' with '_'
            for (auto &c : dumpPath) if (c == '/') c = '_';
            dumpPath = "/tmp/" + dumpPath + "_sfdisk_dump.sfdisk";
            std::string dump_cmd = "sudo sfdisk -d " + device + " > " + dumpPath + " 2>&1";
            std::string dump_out = Terminalexec::execTerminalv2(dump_cmd.c_str());

            // sfdisk -d writes to stdout redirected; if file was created, inform the user.
            std::ifstream fcheck(dumpPath);
            if (fcheck) {
                std::cout << "Partition-table dump written to: " << dumpPath << "\n";
                Logger::log("[INFO] Partition-table dump saved: " + dumpPath + " for device: " + device + " -> partitionrecovery()");
            } else {
                std::cout << "[Warning] Could not write partition-table dump. Output:\n" << dump_out << "\n";
                Logger::log("[WARN] Failed to write partition-table dump for device: " + device + " -> partitionrecovery()");
            }
        }

        std::cout << "\nNotes:\n";
        std::cout << " - The tool printed the partition table above. If you see missing partitions, you can try recovery tools such as 'testdisk' or restore a saved sfdisk dump with 'sudo sfdisk " << device << " < " << (dumpPath.empty() ? "<dump-file>" : dumpPath) << "'.\n";
        std::cout << " - 'testdisk' is interactive; run it manually if you want a guided recovery.\n";

        // Check for testdisk availability and offer to run guidance only
        std::string which_testdisk = Terminalexec::execTerminalv2("which testdisk 2>/dev/null || true");
        if (!which_testdisk.empty()) {
            std::cout << "\nDetected 'testdisk' on the system. This is an interactive tool that can help recover partitions.\n";
            std::cout << "I will not run it automatically. To run it now, open a terminal and run: sudo testdisk " << device << "\n";
        } else {
            std::cout << "\n'testdisk' not found. You can install it (usually package 'testdisk') to perform interactive partition recovery.\n";
        }

        std::cout << "\nPartition recovery helper finished. Review outputs and saved dump before attempting destructive actions.\n";
    }
    
    static void systemrecovery() {
        // ListDrives();
        // std::cout << "\nSystem recovery helper\n";
        // std::cout << "Enter the name of the drive containing the system to inspect (e.g. /dev/sda):\n";
        // std::string device;
        // std::cin >> device;
        // checkDriveName(device);
        std::string device = getAndValidateDriveName("Enter the name of the drive containing the system to inspect (e.g. /dev/sda):");

        std::cout << "\nListing partitions and filesystems for " << device << "\n";
        std::string lsblk_cmd = "lsblk -o NAME,FSTYPE,SIZE,MOUNTPOINT,LABEL -p -n " + device;
        std::string lsblk_out = Terminalexec::execTerminalv2(lsblk_cmd.c_str());
        std::cout << lsblk_out << "\n";
        std::cout << "\nProbing for possible boot partitions (EFI and Linux root candidates)...\n";

        // Find an EFI partition (vfat with esp flag) and a Linux root (ext4/xfs/btrfs)
        std::string blkid_cmd = "sudo blkid -o export " + device + "* 2>/dev/null || true";
        std::string blkid_out = Terminalexec::execTerminalv2(blkid_cmd.c_str());

        // Also show partition flags from parted
        std::string parted_print = Terminalexec::execTerminalv2(("sudo parted -s " + device + " print").c_str());
        std::cout << parted_print << "\n";

        std::cout << "\nIf you want to attempt automatic repair of the bootloader, DriveMgr will prepare a script with suggested steps (it will not run it unless you explicitly allow execution).\n";
        // Build a suggested script (dry-run by default)
        std::string scriptPath = "/tmp/drive_mgr_repair_";
        std::string sanitized = device;
        for (auto &c : sanitized) if (c == '/') c = '_';
        scriptPath += sanitized + ".sh";

        std::ofstream script(scriptPath);
        if (!script) {
            std::cout << "[Error] Could not create helper script at " << scriptPath << "\n";
            Logger::log("[ERROR] Could not create system recovery script: " + scriptPath + " -> systemrecovery()");
            return;
        }

        script << "#!/bin/sh\n";
        script << "# DriveMgr generated helper script: inspect and run manually or allow DriveMgr to run with explicit confirmation.\n";
        script << "# Device: " << device << "\n";
        script << "set -e\n";
        script << "echo 'This script will attempt to mount root and reinstall grub. Inspect before running.'\n";
        script << "# Example sequence (adapt to your partition layout):\n";
        script << "# 1) Mount root partition: sudo mount /dev/sdXY /mnt\n";
        script << "# 2) If EFI: sudo mount /dev/sdXZ /mnt/boot/efi\n";
        script << "# 3) Bind system dirs: sudo mount --bind /dev /mnt/dev && sudo mount --bind /proc /mnt/proc && sudo mount --bind /sys /mnt/sys\n";
        script << "# 4) chroot and reinstall grub: sudo chroot /mnt grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=ubuntu || sudo chroot /mnt grub-install /dev/sdX\n";
        script << "# 5) update-grub inside chroot: sudo chroot /mnt update-grub\n";
        script << "echo 'Script created for guidance only. Do not run without verifying paths.'\n";
        script.close();

        Terminalexec::execTerminalv2((std::string("chmod +x ") + scriptPath).c_str());

        std::cout << "A helper script was created at: " << scriptPath << "\n";
        std::cout << "Open and inspect it. If you want DriveMgr to attempt to run the helper script now, type the exact phrase 'I UNDERSTAND' (all caps) to confirm: ";
        std::string confirmation;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::getline(std::cin, confirmation);

        if (confirmation == "I UNDERSTAND") {
            std::cout << "Running helper script (requires sudo). This is destructive if incorrect.\n";
            Logger::log("[WARN] User allowed DriveMgr to run system recovery helper script: " + scriptPath + " -> systemrecovery()");
            std::string runcmd = std::string("sudo sh ") + scriptPath;
            std::string runout = Terminalexec::execTerminalv2(runcmd.c_str());
            std::cout << runout << "\n";
            std::cout << "Helper script finished. Inspect system state manually.\n";
        } else {
            std::cout << "Not running the helper script. Inspect and run manually if desired: sudo sh " << scriptPath << "\n";
        }
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
        std::cout << "3. recover of system, files,..\n";
        std::cout << "In development...\n";                                                                                                                                                                                                                                                                                                                                                                                                         
        std::cout << "0. Exit/Return to the main menu\n";                                                                                                                                                                                                                                                                                                                                                                                               
        std::cout << "-------------------------------------------\n";
        int forsensicmenuinput;
        std::cin >> forsensicmenuinput;

        switch (static_cast<ForensicMenuOptions>(forsensicmenuinput)) {
            case Info: {
                std::cout << "\n[Info] This is a custom made forensic analysis tool for the Drive Manager\n";
                std::cout << "Its not using actual forsensic tools, but still if its finished would be fully functional\n";
                std::cout << "In development...\n";
                break;
            }
            case CreateDisktImage: {
                CreateDiskImage();
                break;
            }
            case ScanDrive: {
                recovery();
                break;
            }
            case Exit_Return: {
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
            }
            default: {
                std::cout << "[Error] Invalid selection\n";
                return;
            }
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
        // ListDrives();
        // std::cout << "Select a name of a drive to visualize its contents: ";
        // std::string driveName;
        // std::cin >> driveName;
        // checkDriveName(driveName);
        std::string driveName = getAndValidateDriveName("Enter the name of a drive to visualize its contents");

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


class Clone {
    private:
        static void CloneDrive(const std::string &source, const std::string &target) {
            std::cout << "\n[CloneDrive] Do you want to clone data from " << source << " to " << target << "? This will overwrite all data on the target drive(n) (y/n): ";
            char confirmation = 'n';
            std::cin >> confirmation;

            if (confirmation != 'y' && confirmation != 'Y') {
                std::cout << "[Info] Operation cancelled\n";
                Logger::log("[INFO] Operation cancelled -> Clone::CloneDrive()");
                return;

            } else if (confirmation == 'y' || confirmation == 'Y') {
                try {
                    std::string clone_cmd = "sudo dd if=" + source + " of=" + target + " bs=4M status=progress && sync";
                    std::string clone_cmd_output = Terminalexec::execTerminalv2(clone_cmd.c_str());
                    if (clone_cmd_output.find("error") != std::string::npos) {
                        Logger::log("[ERROR] Failed to clone drive from " + source + " to " + target + " -> Clone::CloneDrive()");
                        throw std::runtime_error(clone_cmd_output);
                    }
                    std::cout << "[Success] Drive cloned from " << source << " to " << target << "\n";
                    Logger::log("[INFO] Drive cloned successfully from " + source + " to " + target + " -> Clone::CloneDrive()");
                
                } catch (const std::exception& e) {
                    std::cout << "[Error] Failed to clone drive: " << e.what() << "\n";
                    Logger::log(std::string("[ERROR] Failed to clone drive from ") + source + " to " + target + " -> Clone::CloneDrive(): " + e.what());
                    return;
                }
            }

        }
        
    public:
        static void mainClone() {
            std::string sourceDrive = getAndValidateDriveName("Enter the source drive name to clone");

            std::cout << "\nEnter the target drive name to clone the data on to:\n";
            std::string targetDrive;
            std::cin >> targetDrive;
            checkDriveName(targetDrive);

            if (sourceDrive == targetDrive) {
                Logger::log("[ERROR] Source and target drives are the same -> Clone::mainClone()");
                throw std::runtime_error("Source and target drives cannot be the same!");
                return;
            } else {
                CloneDrive(sourceDrive, targetDrive);
                return;
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
    std::cout << "| Version: 0.8.99-35-experimental\n";
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
    MOUNTUNMOUNT = 10, FORENSIC = 11, DISKSPACEVIRTULIZER = 12, FUNCTION999 = 999, LOGVIEW = 13, CLONEDRIVE = 14
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
        std::cout << "│ 9.  View Info/help                              │\n";
        std::cout << "│10.  Mount/Unmount/restore (ISO/Drives/USB)      │\n";
        std::cout << "│11.  Forensic Analysis (Beta)                    │\n";
        std::cout << "│12.  Disk Space Visualizer (Beta)                │\n";
        std::cout << "│13.  Log viewer                                  │\n";
        std::cout << "│14.  Clone a Drive                               │\n";
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
            case FORMATDRIVE: {
                formatDrive();
                MenuQues(running);
                break;
            }
            case ENCRYPTDECRYPTDRIVE: {
                //EnDecryptDrive();
                DeEncrypting::main();
                MenuQues(running);
                break;
            }
            case RESIZEDRIVE: {
                resizeDrive();
                MenuQues(running);
                break;
            }
            case CHECKDRIVEHEALTH:{
                checkDriveHealth();
                MenuQues(running);
                break;
            }
            case ANALYZEDISKSPACE: {
                analyDiskSpace();
                MenuQues(running);
                break;
            }
            case OVERWRITEDRIVEDATA: {
                std::cout << "[Warning] This function will overwrite the entire data to zeros. Proceed? (y/n)\n";
                char zerodriveinput;
                std::cin >> zerodriveinput;
                if (zerodriveinput == 'y' || zerodriveinput == 'Y') OverwriteDriveData();
                else std::cout << "[Info] Operation cancelled\n";
                MenuQues(running);
                break;
            }
            case VIEWMETADATA: {
                MetadataReader::mainReader();
                MenuQues(running);
                break;
            }
            case VIEWINFO: {
                Info();
                MenuQues(running);
                break;
            }
            case FUNCTION999: {
                break;
            }
            case MOUNTUNMOUNT: {
                MountUtility::mainMountUtil();
                MenuQues(running);
                break;
            }
            case FORENSIC: {
                ForensicAnalysis::mainForensic(running);
                MenuQues(running);
                break;
            }
            case DISKSPACEVIRTULIZER: {
                DSV::DSVmain();
                MenuQues(running);
                break; 
            }
            case EXITPROGRAM: {
                running = false;
                break;
            }
            case LOGVIEW: {
                log_viewer();
                MenuQues(running);
                break;
            }
            case CLONEDRIVE: {
                Clone::mainClone();
                break;
            }
            default: {
                std::cout << "[Error] Invalid selection\n";
                break;
            }
        }
    }
    return 0;
}
