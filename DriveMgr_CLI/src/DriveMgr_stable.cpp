// This is the Stable version of the Program
// v0.8.88-01

#include <iostream>
#include <string>
#include <cstdlib>
#include "include/drivefunctions.h"
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
#include "include/encryption.h"


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
}


void listDrives(std::vector<std::string>& drives) {
    drives.clear();
    std::cout << "\nListing connected drives...\n";
    std::string lsblk = execTerminal("lsblk -o NAME,SIZE,TYPE,MOUNTPOINT -d -n -p");
    std::cout << "\nConnected Drives:\n";
    std::cout << std::left << std::setw(3) << "#" << std::setw(20) << "Device" << std::setw(10) << "Size" << std::setw(10) << "Type" << "Mountpoint" << std::endl;
    std::cout << std::string(60, '-') << "\n";
    std::istringstream iss(lsblk);
    std::string line;
    int idx = 0;
    while (std::getline(iss, line)) {
        if (line.find("disk") != std::string::npos) {
            std::istringstream lss(line);
            std::string device, size, type, mountpoint;
            lss >> device >> size >> type;
            std::getline(lss, mountpoint); 
            if (!mountpoint.empty() && mountpoint[0] == ' ') mountpoint = mountpoint.substr(1);
            std::cout << std::left << std::setw(3) << idx << std::setw(20) << device << std::setw(10) << size << std::setw(10) << type << mountpoint << std::endl;
            drives.push_back(device);
            idx++;
        }
    }
    if (drives.empty()) {
        std::cout << "No drives found!\n";
    }
}

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
    }

    std::string driveName = drives[listpartdrivenum];
    std::cout << "\nPartitions of drive " << driveName << ":\n";
    std::string cmd = "lsblk -o NAME,SIZE,TYPE,MOUNTPOINT -n -p " + driveName;
    std::string output = execTerminal(cmd.c_str());
    std::istringstream iss(output);
    std::string line;
    std::cout << std::left << std::setw(15) << "Name" 
              << std::setw(10) << "Size" 
              << std::setw(10) << "Type" 
              << "Mountpoint" << "\n";
    std::cout << std::string(50, '-') << "\n";

    while (std::getline(iss, line)) {
        if (line.find("part") != std::string::npos) {
            std::cout << line << "\n";
        }
    }
}

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
    std::string output = execTerminal(cmd.c_str());
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
            std::string dfout = execTerminal(dfcmd.c_str());
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
                std::cout << "Are you sure you want to format drive " << drives[driveNumber] << "? (yes/no): ";
                std::string confirmationfd;
                std::cin >> confirmationfd;
                if (confirmationfd == "y" || confirmationfd == "Y" || confirmationfd == "yes" || confirmationfd == "Yes") {
                    std::cout << "Formatting drive: " << drives[driveNumber] << "...\n";
                    std::string execTerminal(("mkfs.ext4 " + drives[driveNumber]).c_str());
                    if (execTerminal.find("error") != std::string::npos) {
                        std::cout << "[Error] Failed to format drive: " << drives[driveNumber] << "\n";
                    } else {
                        std::cout << "Drive formatted successfully: " << drives[driveNumber] << "\n";
                    }
                } else {
                    std::cout << "Formatting cancelled!\n";
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
                    return;
                }
                std::cout << "Are you sure you want to format drive " << drives[driveNumberfd] << " with label '" << label << "' ?\n";
                char confirmationfd;
                std::cin >> confirmationfd;
                if (confirmationfd != 'y' || confirmationfd != 'Y' || confirmationfd != 'yes' || confirmationfd != 'Yes') {
                    std::cout << "[Info] Formating cancled!\n";
                    break;
                }
                std::string execTerminal(("mkfs.ext4 -L " + label + " " + drives[driveNumberfd]).c_str());
                if (execTerminal.find("error") != std::string::npos) {
                    std::cout << "[Error] Failed to format drive: " << drives[driveNumberfd] << "\n";
                } else {
                    std::cout << "Drive formatted successfully with label: " << label << "\n";
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
                std::cout << "Are you sure you want to format drive " << drives[drivenumberfd3] << " with label '" << label << "' and filesystem type '" << fsType << "' ?\n";
                char confirmationfd3;
                std::cin >> confirmationfd3;
                if (confirmationfd3 != 'y' || confirmationfd3 != 'Y' || confirmationfd3 != 'yes' || confirmationfd3 != 'Yes') {
                    std::cout << "[Info] Formating cancelled!\n";
                    return;
                }
                std::string execTerminal(("mkfs." + fsType + " -L " + label + " " + drives[drivenumberfd3]).c_str());
                if (execTerminal.find("error") != std::string::npos) {
                    std::cout << "[Error] Failed to format drive: " << drives[drivenumberfd3] << "\n";
                } else {
                    std::cout << "Drive formatted successfully with label: " << label << " and filesystem type: " << fsType << "\n";
                }
            }
            break;
        default:
            std::cout << "[Error] Invalid option selected.\n";
    }
}

int checkDriveHealth() {
    std::vector<std::string> drives;
    listDrives(drives);
    if (drives.empty()) {
        std::cout << "No drives found!\n";
        return 1;
    }
    for (size_t i = 0; i < drives.size(); ++i) {
        std::cout << i << ": " << drives[i] << "\n";
    }
    std::cout << "\nEnter drive number to check Health:\n";
    int driveNumber_health;
    std::cin >> driveNumber_health;
    if (driveNumber_health < 0 || driveNumber_health >= (int) drives.size()) {
        std::cout << "[Error] Invalid selection!\n";
        return 1;
    }
    if (checkDriveHealth(drives[driveNumber_health])) {
        std::cout << "Drive " << drives[driveNumber_health] << " is healthy.\n";
    } else {
        std::cout << "Drive " << drives[driveNumber_health] << " has issues.\n";
    }
   return 1;
}

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
    }
}

const std::string KEY_STORAGE_PATH = std::string(getenv("HOME")) + "/.var/app/DriveMgr/keys.savekey";

struct EncryptionInfo {
    std::string driveName;
    unsigned char key[32];  // 256-bit key
    unsigned char iv[16];   // 128-bit IV for CBC mode
};

void saveEncryptionInfo(const EncryptionInfo& info) {
    std::ofstream file(KEY_STORAGE_PATH, std::ios::app | std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open key storage file\n";
        return;
    }
    
    char driveName[256] = {0};
    strncpy(driveName, info.driveName.c_str(), 255);
    file.write(driveName, sizeof(driveName));
    file.write((char*)info.key, sizeof(info.key));
    file.write((char*)info.iv, sizeof(info.iv));
    file.close();
}

bool loadEncryptionInfo(const std::string& driveName, EncryptionInfo& info) {
    std::ifstream file(KEY_STORAGE_PATH, std::ios::binary);
    if (!file) {
        std::cerr << "[Error] Cannot open key storage file\n";
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

void generateKeyAndIV(unsigned char* key, unsigned char* iv) {
    if (!RAND_bytes(key, 32) || !RAND_bytes(iv, 16)) {
        throw std::runtime_error("[Error] Failed to generate random key/IV");
    }
}

void encryptDrive(const std::string& driveName) {
    EncryptionInfo info;
    info.driveName = driveName;
    generateKeyAndIV(info.key, info.iv);
    saveEncryptionInfo(info);
    std::stringstream ss;
    ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
       << "--key-file <(echo -n '" << std::string((char*)info.key, 32) << "') "
       << "open " << driveName << " encrypted_" << basename(driveName.c_str());
    
    std::string output = execTerminal(ss.str().c_str());
    if (output.find("Command failed") != std::string::npos) {
        std::cerr << "Encryption failed: " << output << "\n";
        return;
    }
    
    std::cout << "Drive encrypted successfully. The decryption key is stored in " << KEY_STORAGE_PATH << "\n";
}

void decryptDrive(const std::string& driveName) {
    EncryptionInfo info;
    if (!loadEncryptionInfo(driveName, info)) {
        std::cerr << "Error: No encryption key found for " << driveName << "\n";
        return;
    }
    
    // Create dm-crypt mapping for decryption
    std::stringstream ss;
    ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
       << "--key-file <(echo -n '" << std::string((char*)info.key, 32) << "') "
       << "open " << driveName << " decrypted_" << basename(driveName.c_str());
    
    std::string output = execTerminal(ss.str().c_str());
    if (output.find("Command failed") != std::string::npos) {
        std::cerr << "Decryption failed: " << output << "\n";
        return;
    }
    
    std::cout << "Drive decrypted successfully.\n";
}

// main
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

    // Check if drive exists
    bool driveFound = false;
    for (const auto& drive : drives) {
        if (drive == driveName) {
            driveFound = true;
            break;
        }
    }
    if (!driveFound) {
        std::cout << "[Error] Drive " << driveName << " not found!\n";
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
            return;
        }
        
        std::cout << "[Process] Proceeding with encryption of " << driveName << "...\n";
        
        EncryptionInfo info;
        info.driveName = driveName;
        
        // Generate random key and IV for AES-256 CBC
        if (!RAND_bytes(info.key, 32) || !RAND_bytes(info.iv, 16)) {
            std::cout << "[Error] Failed to generate encryption key\n";
            return;
        }
        saveEncryptionInfo(info);

        // Create dm-crypt mapping with custom name
        std::cout << "\n[Input] Enter a name for the encrypted device (e.g., encrypted_drive): ";
        std::string deviceNameEncrypt;
        std::cin >> deviceNameEncrypt;
        
        std::stringstream ss;
        ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
           << "--key-file <(echo -n '" << std::string((char*)info.key, 32) << "') "
           << "open " << driveName << " " << deviceNameEncrypt;
        
        std::string output = execTerminal(ss.str().c_str());
        if (output.find("Command failed") != std::string::npos) {
            std::cout << "[Error] Failed to encrypt the drive: " << output << "\n";
        } else {
            std::cout << "[Success] Drive " << driveName << " has been encrypted as " << deviceNameEncrypt << "\n";
            std::cout << "[Info] The decryption key is stored in " << KEY_STORAGE_PATH << "\n";
        }
    } else if (endecryptselect == 'd' || endecryptselect == 'D') {
        std::cout << "[Warning] Are you sure you want to decrypt " << driveName << "? (y/n)\n";
        char decryptconfirm;
        std::cin >> decryptconfirm;
        if (decryptconfirm != 'y' && decryptconfirm != 'Y') {
            std::cout << "[Info] Decryption cancelled.\n";
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
            return;
        }
        
        std::cout << "[Process] Loading encryption key for " << driveName << "...\n";
        
        EncryptionInfo info;
        if (!loadEncryptionInfo(driveName, info)) {
            std::cout << "[Error] No encryption key found for " << driveName << "\n";
            return;
        }
        
        std::cout << "[Input] Enter the name of the encrypted device to decrypt: ";
        std::string deviceNameDecrypt;
        std::cin >> deviceNameDecrypt;
        
        // Create dm-crypt mapping for decryption
        std::stringstream ss;
        ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
           << "--key-file <(echo -n '" << std::string((char*)info.key, 32) << "') "
           << "open " << driveName << " " << deviceNameDecrypt << "_decrypted";
        
        std::string output = execTerminal(ss.str().c_str());
        if (output.find("Command failed") != std::string::npos) {
            std::cout << "[Error] Failed to decrypt the drive: " << output << "\n";
        } else {
            std::cout << "[Success] Drive " << driveName << " has been decrypted\n";
        }
    } else {
        std::cout << "[Error] Invalid action! Use 'e' for encrypt or 'd' for decrypt\n";
        return;
    }
} // End of encryption and decryption

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
    std::cout << "Are you sure you want to overwrite the drive " << driveName << "?\n";
    char confirmationzerodrive;
    std::cin >> confirmationzerodrive;
    if (confirmationzerodrive == 'y' || confirmationzerodrive == 'Y' || confirmationzerodrive == 'yes' || confirmationzerodrive == 'Yes') {
        char randomconfirmationkey[] = {'a', 'A', 'b', 'B', 'c', 'C', 'd', 'D', 'e', 'E', 'f', 'F', 'g', 'G', 'h', 'H', 'i', 'I', 'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N', 'o', 'O', 'p', 'P', 'q', 'Q', 'r', 'R', 's', 'S', 't', 'T', 'u', 'U', 'v', 'V', 'w', 'W', 'x', 'X', 'y', 'Y', 'z', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        for (int i = 0; i < 10; i++) {
            int randomIndex = rand() % (sizeof(randomconfirmationkey) / sizeof(randomconfirmationkey[0]));
            std::cout << randomconfirmationkey[randomIndex];
        }
        std::cout << "\nPlease enter the confirmationkey to preceed with the operation:\n";
        std::cout << randomconfirmationkey << "\n";
        char randomconfirmationkeyinput[10];
        std::cin >> randomconfirmationkeyinput;
        if (std::string(randomconfirmationkeyinput) == std::string(randomconfirmationkey)) {
            std::cout << "Proceeding with Overwriting " << driveName << "...\n";
            std::string devrandom = execTerminal2ZeroDrive("sudo dd if=/dev/urandom of=" + driveName + " bs=1M status=progress");
            std::string devZero = execTerminal2ZeroDrive("sudo dd if=/dev/zero of=" + driveName + " bs=1M status=progress");
            if (devZero.empty() && devrandom.empty()) {
                std::cout << "[Error] Failed to overwrite the drive\n";
            } else if (devZero.empty() || devrandom.empty()) {
                std::cout << "[Warning] the drive was not completely overwriten, please check the drive and try again if necessary\n";
            } else {
                std::cout << driveName << "'s data has been overwriten successfully\n";
            }
        } else {
            std::cout << "[Error] Invalid confirmation of the Key or unexpected error\n";
        }
    } else {
        std::cout << "Overwriting cancelled\n";
    }
}





void Info() {
    std::cout << "\n----------- Info -----------\n";
    std::cout << "Welcome to Drive Manager, this is a porgram for linux to view, operate,... your Drives in your system\n";
    std::cout << "Warning! You should know some basic things about drives so you dont loose any data\n";
    std::cout << "If you found any problems, visit my Github page and send an issue template\n";
    std::cout << "Basic info:\n";
    std::cout << "Version: 0.8.88-01\n";
    std::cout << "Github: https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux\n";
    std::cout << "Author: Dogwalker-kryt\n";
    std::cout << "----------------------------\n";
}

int main() {
    std::string clear = execTerminal("clear");
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
    std::cout << "9. View Info\n";
    std::cout << "0. Exit\n";
    std::cout << "--------------------------------\n";
    int menuinput;
    std::cin >> menuinput;
    switch (menuinput) {
        case 1: {
            std::vector<std::string> drives;
            listDrives(drives);
            std::cout << "\nPress '1' for returning to the main menu, '2' for advnaced listing or '3' to exit\n";
            int menuques;
            std::cin >> menuques;
            if (menuques == 1) {
                main();
            } else if (menuques == 2) {
                listpartisions(drives);
                std::cout << "\nPress '1' for returning to the main menu, '2' to exit\n";
                int menuques2_2;
                std::cin >> menuques2_2;
                if (menuques2_2 == 1) {
                    main();
                } else if (menuques2_2 == 2) {
                    return 0;
                } else {
                    std::cout << "[Error] Wrong input\n";
                    return 1;
                }
            } else if (menuques == 3) {
                return 0;
            } else {
                std::cout << "[Error] Wrong input";
                return 1;
            }
            break;
        }
        case 2:{
            formatDrive();
            std::cout << "\nPress '1' for returning to the main menu, '2' to exit\n";
            int menuques1;
            std::cin >> menuques1;
            if (menuques1 == 1) {
                main();
            } else if (menuques1 == 2) {
                return 0;
            } else {
                std::cout << "[Error] Wrong input\n";
                return 1;
            }
            break;
        }
        case 3:
            EnDecryptDrive();
            std::cout << "\nPress '1' for returning to the main menu, '2' to exit\n";
            int menuques2;
            std::cin >> menuques2;
            if (menuques2 == 1) {
                main();
            } else if (menuques2 == 2) {
                return 0;
            } else {
                std::cout << "[Error] Wrong input";
                return 1;
            }
            break;
        case 4:
            resizeDrive();
            std::cout << "\nPress '1' for returning to the main menu, '2' to exit\n";
            int menuques3;
            std::cin >> menuques3;
            if (menuques3 == 1) {
                main();
            } else if (menuques3 == 2) {
                return 0;
            } else {
                std::cout << "[Error] Wrong input";
                return 1;
            }
            break;
        case 5:
            checkDriveHealth();
            std::cout << "\nPress '1' for returning to the main menu, '2' to exit\n";
            int menuques4;
            std::cin >> menuques4;
            if (menuques4 == 1) {
                main();
            } else if (menuques4 == 2) {
                return 0;
            } else {
                std::cout << "[Error] Wrong input";
                return 1;
            }
            break;
        case 6:{
            analyDiskSpace();
            std::cout << "\nPress '1' for returning to the main menu, '2' to exit\n";
            int menuques6;
            std::cin >> menuques6;
            if (menuques6 == 1) {
                main();
            } else if (menuques6 == 2) {
                return 0;
            } else {
                std::cout << "[Error] Wrong input";
                return 1;
            }
            break;
        }
        case 7:{
            std::cout << "[Warning] This funcion is Overwriting the hole data to Zeros\n"; 
            std::cout << "if you dont know what you are doing then dont use this function for no data loss\n";
            std::cout << "Do you want to proceed?\n";
            char zerodriveinput;
            if (zerodriveinput == 'y' || zerodriveinput == 'Y' || zerodriveinput == 'yes' || zerodriveinput == 'Yes') {
                OverwriteDriveData();
            } else {
                main();
            }
	        std::cout << "\nPress '1' to return to the main menu or '2' for exit\n";
	        int menuques7;
	        std::cin >> menuques7;
	        if (menuques7 == 1) {
		        main();
	        } else if (menuques7 == 2) {
		        return 0;
	        }
            break; 
        }
        case 8:{
            std::cout << "This fuction is not implemented yet!\n";
	        std::cout << "\nPress '1' to return to the main menu or '2' for exit\n";
	        int menuques8;
	        if (menuques8 == 1) {
		        main(); 
	        } else if (menuques8 == 2) {
		        return 0;
	        } 
        }
        case 9:
            Info();
	        std::cout << "\nPress '1' to return to the main menu or '2' for exit\n";
	        int infoques;
	        std::cin >> infoques;
	        if (infoques == 1) {
		        main();
	        } else if (infoques == 2) {
		        return 0;
	        }
            break;
        case 0:
            std::cout << "Exiting DriveMgr\n";
            return 0;
        default:
            std::cout << "[Error] Invalid selection\n";
            return 1;
    }
    return 1;
}
