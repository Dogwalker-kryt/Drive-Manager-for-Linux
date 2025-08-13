#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#ifdef __cplusplus
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <array>
#include <limits>
#include <iomanip>
#include <functional>

// Basic drive information structure
namespace gui {

struct DriveInfo {
    std::string device;
    std::string size;
    std::string type;
    std::string mountpoint;
    std::string label;
    std::string fstype;
    bool isEncrypted;
};

struct SpaceInfo {
    uint64_t totalSpace;
    uint64_t usedSpace;
    uint64_t freeSpace;
    std::vector<std::pair<std::string, uint64_t>> topDirectories;
};

struct DriveHealth {
    bool isHealthy;
    int temperature;
    int powerOnHours;
    int reallocatedSectors;
    std::string smartStatus;
    std::vector<std::pair<std::string, std::string>> smartAttributes;
};

// Core functions
std::vector<DriveInfo> listDrives();

} // namespace gui

extern "C" {
#endif

#include <stdint.h>
#include "drivefunctions.h"
#include "encryption.h"

#ifdef __cplusplus
// GUI functions (C++ interface)
    std::string device;
    std::string size;
    std::string type;
    std::string mountpoint;
    std::string label;
    std::string fstype;
    bool isEncrypted;
};

std::vector<DriveInfo> listDrives();

namespace gui {

bool formatDrive(const DriveInfo& drive, const char* fstype, const char* label, 
                std::function<void(int)> progress_callback = nullptr);

bool encryptDrive(const char* device, const char* password, 
                 std::function<void(double)> progress_callback = nullptr);

bool decryptDrive(const char* device, const char* password,
                 std::function<void(double)> progress_callback = nullptr);

bool zeroDrive(const char* device, std::function<void(double)> progress_callback = nullptr);

SpaceInfo analyzeSpace(const char* mountpoint);

DriveHealth checkDriveHealth(const char* device);

} // namespace gui
#endif // __cplusplus

// C interface
#ifndef __cplusplus
typedef struct gui_DriveInfo gui_DriveInfo;
typedef struct gui_SpaceInfo gui_SpaceInfo;
typedef struct gui_DriveHealth gui_DriveHealth;
#endif

// These functions are callable from both C and C++
#ifdef __cplusplus
extern "C" {
#endif

void gui_listDrives(gui_DriveInfo** drives, int* count);
void gui_freeDrives(gui_DriveInfo* drives, int count);

bool gui_formatDrive(const char* device, const char* fstype, const char* label);
bool gui_encryptDrive(const char* device, const char* password);
bool gui_decryptDrive(const char* device, const char* password);
bool gui_zeroDrive(const char* device);

gui_SpaceInfo* gui_analyzeSpace(const char* mountpoint);
void gui_freeSpaceInfo(gui_SpaceInfo* info);

gui_DriveHealth* gui_checkDriveHealth(const char* device);
void gui_freeDriveHealth(gui_DriveHealth* health);

#ifdef __cplusplus
}
#endif

#endif // FUNCTIONS_H