# Drive Manager for Linux

A terminal-based drive management tool for Linux systems, written in C++. The goal of this project is to provide an easy-to-use interface for managing drives directly from the terminal, with plans for future GUI support.

Current version in stable: 0.8.79

Current version in experimental: 0.8.88-01

always check if a new version has been released


> **Warning**  
> This tool is intended for users who understand the risks involved with formatting, encrypting, or otherwise modifying drives. Always back up your data and double-check your selections!

## Files
in the current state of the repo, there are two folders: include (with all .h files) and src (with the source code files. All files to be needed are in the corosponding folders.
```
├─/DriveMgr_CLI
|   ├─/include
|   |  ├─/drivefunctions.h
|   |  └─/encryption.h
|   ├─/src
|   |   ├─/DriveMgr_experi.cpp
|   |   └─/DriveMgr_stable.cpp
├─/DriveMgr_GUI
|   ├─/include
|   ├─/build_src
|   └─/src 
```
in the other file outside the folder is the newest version (beta), this version will not be bug free, but atleast compilable


! If you dowload one of the source code files you need to have the header files drivefunctions.h and encryption.h with it !


## Features

- **List all drives**: View all drives currently connected to your system.
- **Format drives**: Format drives with optional label and filesystem type (e.g., ext4).
- **Encrypt/Decrypt drives**: (doesnt realy work, im trying to make the encryption with AES-256) Secure your drives using encryption. _Warning: Losing your key means losing your data!_
- **Resize drives**: Adjust the size of existing drives.
- **Check drive health**: Basic S.M.A.R.T. health check of your drives.
- **More features coming soon!**

## Usage

1. **Build the project**  
   Make sure you have a C++ compiler and necessary development tools installed.

   ```sh
   g++ DriveMgr_experi.cpp -I.. -o DriveMgr -lssl -lcrypto 
   ```
   or
   ```sh
   g++ DriveMgr_stable.cpp -I.. -o DriveMgr -lssl -lcrypto 
   ```
   i will also upload the finished executable
3. **Run the program**  
   You need to run as root to manage drives:

   ```sh
   sudo ./DriveMgr
   ```

4. **Follow the menu**  
   The program will present a menu:
   ```
   Welcome to DriveMgr
   ------------- Menu -------------
   1. List drives
   2. Format drive
   3. Encrypt/Decrypt drive
   4. Resize drive
   5. Check drive health
   6. Analyse Disk space
   7. Overwrite Disk Data
   8. Info
   9. Exit
   --------------------------------
   ```

   Enter the number corresponding to the desired operation and follow the prompts.

## Example

- **Listing Drives**  
  Shows all connected drives with their device name, size, type, and mountpoint.

- **Formatting a Drive**  
  You can select a drive, optionally provide a label and filesystem type, and confirm before any irreversible actions are performed.

## Requirements

- Linux system (Debian-based distros tested)
- C++17 or newer
- Root privileges for most operations (Formatinn for example)

## Roadmap

- [x] Implement encryption/decryption
- [x] Implement resizing and health checking
- [ ] Add a GUI version

The GUI version will be in development if the Terminal based version funktions as it should!
## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for more details.

---

**Known Bug/issues:**

en/decrypt function wont en/decrypt USB

**Disclaimer:**  
This tool is under active development. Some features may not be fully implemented or stable. Use with caution, and always test on non-critical drives first!

## Ideas or requests?

send me a mesage/dicussion/request/... for some ideas to improve my tool!
