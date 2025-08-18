# Drive Manager for Linux

A terminal-based drive management tool for Linux systems, written in C++. The goal of this project is to provide an easy-to-use interface for managing drives directly from the terminal, with plans for future GUI support.

Current version in stable: 0.8.88-01

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
- **Encrypt/Decrypt drives**: Secure your drives using encryption. _Warning: Losing your key means losing your data!_
- **Resize drives**: Adjust the size of existing drives.
- **Check drive health**: Basic S.M.A.R.T. health check of your drives.
- **view Partitions adn manage them**:
- **Overwrite the data on a drive**
- **More features coming soon!**

## Usage
1. **download the project**
   copy this command in your Terminal:
   
   ```sh
   git clone https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux
   ```

2. **Build the project**
   
   2.1 **Normal way**
   
   Make sure you have a C++ compiler and necessary development tools installed.

   ```sh
   g++ DriveMgr_experi.cpp -I.. -o DriveMgr -lssl -lcrypto 
   ```
   or
   ```sh
   g++ DriveMgr_stable.cpp -I.. -o DriveMgr -lssl -lcrypto 
   ```
   you need to also create following:
   - ~/.var/app/DriveMgr
   - ~/.var/app/DriveMgr/bin
   - ~/.var/app/DriveMgr/log.txt
   - ~/.var/app/DriveMgr/keys.savekey
   **or do it the easier way**
     
   2.2 **easier way!!!**
   
   you can run the build_src.sh, this script will create all necessery files, compile the executable, install the nessecary build essentials if not installed (openssl, g++ and gtk-4.0), make the directory's it needs. so basicly it does everything for you
   
4. **Run the program**  
   You need to run as root to manage drives:

   ```sh
   sudo ./DriveMgr
   ```

5. **Follow the menu**  
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

This project is licensed under the **GNU General Public License v3.0 (GPL-3.0)**.

You are free to use, modify, and distribute this software, **provided that** any modifications are also licensed under GPL-3.0, and proper attribution is given to the original author(s). If you redistribute the software (modified or unmodified), you must make the source code available and include a copy of this license.

**NO WARRANTY**: This software is provided "as-is," without any warranty of any kind, express or implied, including but not limited to the warranties of merchantability or fitness for a particular purpose. In no event shall the authors or copyright holders be liable for any claim, damages, or other liability arising from, out of, or in connection with the software or the use or other dealings in the software.

For full details of the license, see the [LICENSE](./LICENSE) file.


---

**Known Bug/issues:**

No current bug/issues found in the code

**Disclaimer:**  
This tool is under active development. Some features may not be fully implemented or stable. Use with caution, and always test on non-critical drives first!

## Ideas or requests?

send me a mesage, dicussion, request,... for some ideas to improve my tool!
