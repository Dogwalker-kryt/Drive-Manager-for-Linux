// Drive manager
#include <iostream>
#include <string>
#include <cstdlib>
#include "drivefunk.h"
#include <cstdio>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <array>
#include <limits>
//#include <vector>
//#include <sstream>

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

std::vector<std::string> listDrives() {
    std::vector<std::string> drives;
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
    return drives;
    std::cout << "Press Enter to return to the main menu...\n";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

// Dummy implementations for the other functions (to avoid linker errors)
bool formatDrive(const std::string&, const std::string&, const std::string&) { return false; }
bool encryptDrive(const std::string&, bool) { return false; }
bool resizeDrive(const std::string&, int) { return false; }
bool checkDriveHealth(const std::string&) { return false; }

void formatDrive() {
    std::cout << "Choose an option:\n";
    std::cout << "1. Format drive\n";
    std::cout << "2. Format drive with label\n";
    std::cout << "3. Format drive with label and filesystem\n";
    int fdinput;
    std::cin >> fdinput;
    switch (fdinput) {
        case 1:
            std::cout << "Choose a Drive to Format\n";
            {
                auto drives = listDrives();
                if (drives.empty()) break;
                std::cout << "Enter drive number: ";
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
                } else {
                    std::cout << "Formatting cancelled!\n";
                    return;
                }
            }
            break;
        case 2:
            {
                std::cout << "Enter label: ";
                std::string label;
                std::cin >> label;
                std::cout << "Formatting drive with label: " << label << "\n";

            }
            break;
        case 3:
            {
                std::string label, fsType;
                std::cout << "Enter label: ";
                std::cin >> label;
                std::cout << "Enter filesystem type (e.g. ext4): ";
                std::cin >> fsType;
                std::cout << "Formatting drive with label: " << label << " and filesystem: " << fsType << "\n";
            }
            break;
        default:
            std::cout << "Invalid option selected.\n";
    }
}

int checkDriveHealth() {
    auto drives = listDrives();
    if (drives.empty()) {
        std::cout << "No drives found!\n";
        return 1;
    }
    for (size_t i = 0; i < drives.size(); ++i) {
        std::cout << i << ": " << drives[i] << "\n";
    }
    std::cout << "Enter drive number to check Health:\n";
    int driveNumber_health;
    std::cin >> driveNumber_health;
    if (driveNumber_health < 0 || driveNumber_health >= (int) drives.size()) {
        std::cout << "Invalid selection!\n";
        return 1;
    }
    // Check the health of the selected drive
    if (checkDriveHealth(drives[driveNumber_health])) {
        std::cout << "Drive " << drives[driveNumber_health] << " is healthy.\n";
    } else {
        std::cout << "Drive " << drives[driveNumber_health] << " has issues.\n";
    }
    std::cout << "Press Enter to return to the main menu...\n";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

void resizeDrive() {
    auto drives = listDrives();
    if (drives.empty()) {
        std::cout << "No drives found!\n";
        return;
    }
    std::cout << "Enter drive number to resize:\n";
    int driveNumber_resize;
    std::cin >> driveNumber_resize;
    if (driveNumber_resize < 0 || driveNumber_resize >= (int) drives.size()) {
        std::cout << "Invalid selection!\n";
        return;
    }
    std::cout << "Enter new size in GB for drive " << drives[driveNumber_resize] << ":\n";
    int newSize;
    std::cin >> newSize;
    if (newSize <= 0) {
        std::cout << "Invalid size entered!\n";
        return;
    }
    std::cout << "Resizing drive " << drives[driveNumber_resize] << " to " << newSize << " GB...\n";
    if (resizeDrive(drives[driveNumber_resize], newSize)) {
        std::cout << "Drive resized successfully\n";
    } else {
        std::cout << "Failed to resize drive\n";
    }
    std::cout << "Press Enter to return to the main menu...\n";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}







int main() {
    std::cout << "Welcome to DriveMgr\n";
    std::cout << "------------- Menu -------------\n";
    std::cout << "1. List drives\n";
    std::cout << "2. Format drive\n";
    std::cout << "3. Encrypt/Decrypt drive\n";
    std::cout << "4. Resize drive\n";
    std::cout << "5. Check drive health\n";
    std::cout << "6. Exit\n";
    std::cout << "--------------------------------\n";
    int menuinput;
    std::cin >> menuinput;
    switch (menuinput) {
        case 1:
            listDrives();
            break;
        case 2:
            formatDrive();
            break;
        case 3:
            std::cout << "Function follows...\n";
            break;
        case 4:
            resizeDrive();
            break;
        case 5:
            checkDriveHealth();
            break;
        case 6:
            std::cout << "Exiting DriveMgr\n";
        default:
            std::cout << "Invalid selection\n";
            return 1;
    }
    //return 0;
}
