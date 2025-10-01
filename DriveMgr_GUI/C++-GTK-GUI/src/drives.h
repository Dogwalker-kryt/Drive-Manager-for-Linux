#pragma once
#include <vector>
#include <string>

// Populate a list of drives. Each entry is formatted as:
// NAME|SIZE|TYPE|MOUNT
bool listDrives(std::vector<std::string> &out);

// Get detailed device info for a given device path (e.g. /dev/sda)
// Output is written to 'out'. Returns true on success.
bool getDeviceInfo(const std::string &devpath, std::string &out);

