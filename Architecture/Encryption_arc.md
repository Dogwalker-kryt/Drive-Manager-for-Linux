# Encryption & Decryption System Documentation

## Overview

This module provides a complete AES‑256 encryption/decryption system for drives using `cryptsetup`.  
It includes:

- secure key + IV generation  
- salted XOR‑based obfuscation  
- persistent encrypted key storage  
- drive encryption and decryption helpers  
- user‑facing confirmation workflows  
- logging and error handling  

The system is split into two main components:

1. **EnDecryptionUtils** — low‑level cryptographic utilities  
2. **DeEncrypting** — user‑facing encryption/decryption workflow  

------------------------------------------------------------
# EnDecryptionUtils Class

This class handles all cryptographic operations, key storage, obfuscation, and interaction with `cryptsetup`.

------------------------------------------------------------
## `saveEncryptionInfo()` — Store Key, IV, and Salt

    ```cpp
    static void saveEncryptionInfo(const EncryptionInfo& info)
    ```

This function stores:

- drive name  
- randomly generated salt  
- obfuscated AES key  
- obfuscated IV  

### Steps:

1. Opens the key storage file:

        std::ofstream file(KEY_STORAGE_PATH, std::ios::app | std::ios::binary);

2. Generates a random salt:

        auto salt = generateSalt();

3. Obfuscates key and IV using XOR with salt:

        auto obfKey = obfuscate(info.key, sizeof(info.key), salt.data(), salt.size());

4. Writes drive name (256 bytes, null‑terminated):

        file.write(driveName, sizeof(driveName));

5. Writes salt length + salt:

        file.write(reinterpret_cast<const char*>(&saltLen), sizeof(saltLen));

6. Writes obfuscated key and IV:

        file.write(reinterpret_cast<const char*>(obfKey.data()), obfKey.size());

7. Logs success.

------------------------------------------------------------
## `loadEncryptionInfo()` — Retrieve Stored Key & IV

    ```cpp
    static bool loadEncryptionInfo(const std::string& driveName, EncryptionInfo& info)
    ```

This function loads the stored encryption info for a specific drive.

### Steps:

1. Opens the key storage file:

        std::ifstream file(KEY_STORAGE_PATH, std::ios::binary);

2. Reads entries sequentially:

        file.read(storedDriveName, sizeof(storedDriveName));

3. Reads salt length + salt:

        file.read(reinterpret_cast<char*>(&saltLen), sizeof(saltLen));

4. Reads obfuscated key and IV:

        file.read(reinterpret_cast<char*>(obfKey.data()), 32);

5. Deobfuscates using XOR:

        auto key = deobfuscate(obfKey.data(), 32, salt.data(), saltLen);

6. If drive name matches, fills `info` and returns true.

7. Logs failure if no match found.

------------------------------------------------------------
## `generateKeyAndIV()` — Secure Random Key Generation

    ```cpp
    static void generateKeyAndIV(unsigned char* key, unsigned char* iv)
    ```

Uses OpenSSL:

- 32 bytes for AES‑256 key  
- 16 bytes for IV  

Throws on failure.

------------------------------------------------------------
## `encryptDrive()` — Encrypt a Drive Using cryptsetup

    ```cpp
    static void encryptDrive(const std::string& driveName)
    ```

### Steps:

1. Generates key + IV  
2. Saves them (salted + obfuscated)  
3. Writes key to a secure temporary file:

        chmod(tmpKeyFile.c_str(), 0600);

4. Runs cryptsetup:

        cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 --key-file <tmp> open <drive> encrypted_<name>

5. Deletes temp key file  
6. Logs success or failure  

------------------------------------------------------------
## `decryptDrive()` — Decrypt a Drive Using Stored Key

    ```cpp
    static void decryptDrive(const std::string& driveName)
    ```

### Steps:

1. Loads stored key + IV  
2. Writes key to secure temp file  
3. Runs cryptsetup:

        cryptsetup ... open <drive> decrypted_<name>

4. Deletes temp key file  
5. Logs success or failure  

------------------------------------------------------------
## Salt & Obfuscation Helpers

### `generateSalt()`

    ```cpp
    static std::vector<unsigned char> generateSalt(size_t length = 16)
    ```

Generates cryptographically secure random salt.

### `obfuscate()` / `deobfuscate()`

    ```cpp
    result[i] = data[i] ^ salt[i % saltLen];
    ```

Simple XOR‑based obfuscation.

------------------------------------------------------------
# DeEncrypting Class

This class provides the **interactive user workflow** for encryption and decryption.

------------------------------------------------------------
## `encrypting()` — User‑Driven Encryption Workflow

    ```cpp
    static void encrypting()
    ```

### Steps:

1. User selects drive via TUI:

        std::string driveName = listDrives(true);

2. User confirms encryption:

        Are you sure? (y/n)

3. User must enter a random confirmation key  
4. Generates key + IV  
5. Saves encryption info  
6. Asks for encrypted device name  
7. Runs cryptsetup  
8. Logs success  

------------------------------------------------------------
## `decrypting()` — User‑Driven Decryption Workflow

    ```cpp
    static void decrypting()
    ```

### Steps:

1. User selects drive  
2. User confirms  
3. User enters confirmation key  
4. Loads stored key  
5. Asks for decrypted device name  
6. Runs cryptsetup  
7. Logs success  

------------------------------------------------------------
## `main()` — Entry Point for Encryption/Decryption Menu

    ```cpp
    static void main()
    ```

Prompts the user:

    Do you want to 'e'ncrypt or 'd'ecrypt?

Dispatches to:

- `encrypting()`  
- `decrypting()`  

Handles invalid input gracefully.

------------------------------------------------------------
# Summary

The encryption/decryption subsystem provides:

- AES‑256 encryption using dm‑crypt  
- secure key generation  
- salted XOR obfuscation  
- persistent key storage  
- safe temporary key handling  
- interactive user confirmations  
- full logging  
- robust error handling  

It is designed to be **secure**, **traceable**, and **user‑friendly**, while preventing accidental data loss.

------------------------------------------------------------
