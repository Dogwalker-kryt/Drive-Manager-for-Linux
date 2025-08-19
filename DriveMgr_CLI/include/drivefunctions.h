
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
    static void log(const std::string& operation);
};

// execTerminal functions
std::string execTerminal2ZeroDrive(const std::string &command);
std::string execTerminal(const char* cmd);

// Partition management functions
bool resizePartition(const std::string& device, uint64_t newSizeMB);
bool movePartition(const std::string& device, int partNum, uint64_t startSectorMB);
bool changePartitionType(const std::string& device, int partNum, const std::string& newType);
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
