#include "drives.h"
#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>

static std::string run_cmd(const std::string &cmd) {
    std::array<char, 4096> buf;
    std::string result;
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buf.data(), buf.size(), pipe) != nullptr) {
        result += buf.data();
    }
    pclose(pipe);
    return result;
}

bool listDrives(std::vector<std::string> &out) {
    out.clear();
    // Use lsblk to get NAME,SIZE,TYPE,MOUNTPOINT in a simple parseable form
    std::string cmd = "lsblk -dn -o NAME,SIZE,TYPE,MOUNTPOINT --pairs";
    std::string raw = run_cmd(cmd);
    if (raw.empty()) return false;
    std::istringstream iss(raw);
    std::string line;
    while (std::getline(iss, line)) {
        // lines like: NAME="sda" SIZE="238.5G" TYPE="disk" MOUNTPOINT=""
        std::string name, size, type, mount;
        auto extract = [&](const std::string &key)->std::string{
            auto pos = line.find(key + "=\"");
            if (pos==std::string::npos) return std::string();
            pos += key.size()+2;
            auto end = line.find('"', pos);
            if (end==std::string::npos) return std::string();
            return line.substr(pos, end-pos);
        };
        name = extract("NAME");
        size = extract("SIZE");
        type = extract("TYPE");
        mount = extract("MOUNTPOINT");
        if (name.empty()) continue;
        std::ostringstream outl;
        outl << name << "|" << (size.empty()?"?":size) << "|" << (type.empty()?"?":type) << "|" << (mount.empty()?"":mount);
        out.push_back(outl.str());
    }
    return true;
}

bool getDeviceInfo(const std::string &devpath, std::string &out) {
    out.clear();
    // Run lsblk -f and blkid to provide human-friendly info
    std::string ls = run_cmd(std::string("lsblk -o NAME,FSTYPE,LABEL,UUID,MOUNTPOINT,SIZE -f ") + devpath);
    std::string blk = run_cmd(std::string("blkid ") + devpath + " 2>/dev/null");
    std::ostringstream oss;
    oss << "lsblk output:\n" << ls << "\n";
    if (!blk.empty()) oss << "blkid:\n" << blk << "\n";
    // smartctl may not be available; try it but ignore failures
    std::string smart = run_cmd(std::string("smartctl -i ") + devpath + " 2>/dev/null");
    if (!smart.empty()) oss << "smartctl:\n" << smart << "\n";
    out = oss.str();
    return !out.empty();
}

