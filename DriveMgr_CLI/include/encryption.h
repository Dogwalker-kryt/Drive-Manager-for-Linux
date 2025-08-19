#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <string>
#include <fstream>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <vector>
#include <ostream>
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

// encryption
const std::string KEY_STORAGE_PATH = std::string(getenv("HOME")) + "/.var/app/DriveMgr/keys.savekey";

struct EncryptionInfo {
    std::string driveName;
    unsigned char key[32];  // 256-bit key
    unsigned char iv[16];   // 128-bit IV for CBC mode
};

void saveEncryptionInfo(const EncryptionInfo& info);

bool loadEncryptionInfo(const std::string& driveName, EncryptionInfo& info);

void generateKeyAndIV(unsigned char* key, unsigned char* iv); 


#endif 
