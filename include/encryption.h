#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <string>
#include <vector>

// Structure to store encryption info
struct EncryptionInfo {
    std::string driveName;
    unsigned char key[32];  // 256-bit key
    unsigned char iv[16];   // 128-bit IV for CBC mode
};

// Main function to handle encryption/decryption
void encryptDecryptDrive(const std::vector<std::string>& drives);

// Individual functions for encryption and decryption
void encryptDrive(const std::string& driveName);
void decryptDrive(const std::string& driveName);

#endif 
