#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <openssl/rand.h>

// encryption
const std::string KEY_STORAGE_PATH = std::string(getenv("HOME")) + "/.var/app/DriveMgr/keys.savekey";

struct EncryptionInfo {
    std::string driveName;
    unsigned char key[32];  // 256-bit key
    unsigned char iv[16];   // 128-bit IV for CBC mode
};

void saveEncryptionInfo(const EncryptionInfo& info) {
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

bool loadEncryptionInfo(const std::string& driveName, EncryptionInfo& info) {
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

void generateKeyAndIV(unsigned char* key, unsigned char* iv) {
    if (!RAND_bytes(key, 32) || !RAND_bytes(iv, 16)) {
        throw std::runtime_error("[Error] Failed to generate random key/IV");
        Logger::log("[ERROR] Failed to generate random key/IV for encryption -> generateKeyAndIV()");
    }
}

#endif 
