# Drive Management Utiliy for Linux

A terminal-based drive management tool for Linux systems, written in C++. The goal of this project is to provide an easy-to-use interface for managing drives directly from the terminal, with plans for future GUI support.

## Version Control

### CLI:

Current version in stable: v0.8.88-08

Current version in experimental: v0.8.98-98

### GUI:

Current Rust GUI: v0.1.5-alpha (this version is not really funtional, but the list dirves works)

### Note:
- always check if a new version has been released
- Help for the GUI verion is appreciated


> **Warning**  
> This tool is intended for users who understand the risks involved with formatting, encrypting, or otherwise modifying drives. Always back up your data and double-check your selections!
> Also if there stnds somehting like 'enter drive name:' dont type thing in like '/dev/sda rm -rf' THIS WILL RESOLVE IN DATA LOSS

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
├─/build_src.sh
├─/bin
|   └─/Here are all finished builds (executables)
```
in the other file outside the folder is the newest version (beta), this version will not be bug free, but atleast compilable


## Features

- **List all drives**: View all drives currently connected to your system.
- **Format drives**: Format drives with optional label and filesystem type (e.g., ext4).
- **Encrypt/Decrypt drives**: Secure your drives using encryption. _Warning: Losing your key means losing your data!_
- **Resize drives**: Adjust the size of existing drives.
- **Check drive health**: Basic S.M.A.R.T. health check of your drives.
- **view Partitions and manage them**:
- **Overwrite the data on a drive**
- **Active Logging**: for runtime debugging, histroy of waht things you done, if somthing brakes
- **Metadata Reader**
- **Forensic tools (still has bug/porblmes, but is in development)**
- **Diskspace visulizer (still has bug/porblmes, but is in development)**
- **More features coming soon! or aim to layz to add them in the Readme**

## Usage
1. **download the project**
    
   copy this command in your Terminal:
   
   ```sh
   git clone https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux
   ```

3. **Build the project**
   
   2.1 **Normal way**
   
   Make sure you have a C++ compiler and necessary development tools installed.

   ```sh
   g++ DriveMgr_experi.cpp -I.. -o DriveMgr -lssl -lcrypto 
   ```
   or
   ```sh
   g++ DriveMgr_stable.cpp -I.. -o DriveMgr -lssl -lcrypto 
   ```
   or for GUI
   ```sh
   cargo build
   ```
   you need to also create following:
   - ~/.var/app/DriveMgr
   - ~/.var/app/DriveMgr/bin
   - ~/.var/app/DriveMgr/log.dat
   - ~/.var/app/DriveMgr/keys.bin
     
   **or do it the easier way**
     
   2.2 **easier way)**
   you can run the build_src.sh, this script will create all necessery files, compile the executable, install the nessecary build essentials if not installed (openssl, g++ and 	gtk-4.0), make the directory's it needs. so basicly it does everything for you. **Package Manager Compatability:** apt, dnf, pacman, zypper.

   Note: the build script can fail if that happens you should try making ervting your self. If there is any missmatch, please open an issue!

	The build_src.sh, will ask some questions if this, if that and so. you can also run it with flags like 
	```sh
	--dry-run # this will only ask the question but doesnt actualy do anythig
 	--no-install # does not attempt to install missing packages
 	--targets=  # this will only build teh prefered type e.g. cli, gui
 	-h, --help # for help what wich opetion does like --dry-run
 	```

   ```sh
   bash build_src.sh [option if wanted]
   ```
   
5. **Run the program**  
   You need to run as root to manage drives:

   ```sh
   sudo ./DriveMgr_(the version name, like satble, gui, experi)
   ```
   for GUI 
   ```sh
   cargo run
   ```

   **or**
   ```sh
   dmgrctl
   ```
	use the command that will be made if you agree to (avilable in the next update in the build_src.sh)
   
7. **Follow the menu**  
   The program will present a menu:
   ```
    ┌─────────────────────────────────────────────────┐
    │              DRIVE MANAGEMENT UTILITY           │
    ├─────────────────────────────────────────────────┤
    │ 1.  List Drives                                 │
    │ 2.  Format Drive                                │
    │ 3.  Encrypt/Decrypt Drive (AES-256)             │
    │ 4.  Resize Drive                                │
    │ 5.  Check Drive Health                          │
    │ 6.  Analyze Disk Space                          │
    │ 7.  Overwrite Drive Data                        │
    │ 8.  View Drive Metadata                         │
    │ 9.  View Info                                   │
    │10.  Mount/Unmount (ISO/Drives)                  │
    │11.  Forensic Analysis (Beta)                    |
    │13.                                              │
    │ 0.  Exit                                        │
    └─────────────────────────────────────────────────┘
    choose an option [0 - 12]:
   ```
To use it enter the number of an function you want to use on some dangerious function like Overwriting the data are security fucntion like you must enter a generated key if you are really sure if you want to do that. 

## Simple guide how to use the Drive Manager


### Step 1: Start the Program
```sh
sudo ./DriveMgr_experi_v0.8.89-12
```
Note: sudo is optional, because for some function the porgram doesnt need sudo access.


### Step 2: Choose and Option
```sh
    ┌─────────────────────────────────────────────────┐
    │              DRIVE MANAGEMENT UTILITY           │
    ├─────────────────────────────────────────────────┤
    │ 1.  List Drives                                 │
    │ 2.  Format Drive                                │
    │ 3.  Encrypt/Decrypt Drive (AES-256)             │
    │ 4.  Resize Drive                                │
    │ 5.  Check Drive Health                          │
    │ 6.  Analyze Disk Space                          │
    │ 7.  Overwrite Drive Data                        │
    │ 8.  View Drive Metadata                         │
    │ 9.  View Info                                   │
    │10.  Mount/Unmount (ISO/Drives)                  │
    │11.  Forensic Analysis (Beta)                    |
    │13.                                              │
    │ 0.  Exit                                        │
    └─────────────────────────────────────────────────┘
    choose an option [0 - 12]:
```
lets choose for example 'View Metadata of a Drive', for this enter the number on the side, in this case its ``` 8 ``` 


## Step 3: Choose a Drive

the number of Drives, size, name,... can vary from storage device to storage device
```sh
Listing connected drives...

Connected Drives:
#  Device            Size      Type      Mountpoint     FSType    Status
------------------------------------------------------------------------------------------
0  /dev/sda          1T        disk                               Unknown filesystem

Enter number of the drive you want to see the metadata of: 
```
Now enter the number of the drive on the left side, in some fucntions the program will need the actual name of the drive so for example /dev/sda. Just read the prompt correctly.

So we enterd ``` 0 ```.

### Step 4: The Actions
This step can vary form function to function, but everything you need to do and so are well writen in the program.

Now we can see the progam read the Metadate of my Drive ``` /dev/sda ```
```sh
-------- Drive Metadata --------
Name:       /dev/sda
Size:       1T
Model:      (The full drive model name)
Serial:     (your_serialnumber_of_your_drive)
Type:       disk
Mountpoint: Not mounted
Vendor:     (your_vendor_of_the_drive)     
Filesystem: N/A (here the program couldnt read this part of the metadata)
UUID:       N/A (here the program couldnt read this part of the metadata)

-------- SMART Data --------
(if you have smartmontools installed, here would be even more metadata)
------------------------------
[Error] Unable to open log file: ~/.var/app/DriveMgr/log.dat Reason: No such file or directory (here we see a error from some side function)

Press '1' for returning to the main menu, '2' to exit

```
### Step 5: End/Return Phase
When the action you choose is finished, you will be given this promt
```sh
Press '1' for returning to the main menu, '2' to exit
```
what it does speaks for it self.

If you return to the menu the program will clear the terminal so it looks clean, but dont worry the program should log everything have done before.

if you are in a function like Overwriting the Drive data and you dont want to do this action, just hit **Ctrl + c**, this will force to end the porgram
  
**Note: this tool should be easy to use, but you should also know what you are doing**


## Requirements

- Linux system (Debian-based distros)
- C++17 or newer CLI
- Root privileges for most operations (Formating for example)
- build-essetials, like openssl, g++, gtk-4.0 and S.M.A.R.T tools
- rustc compiler for GUI

## Roadmap

- [x] Implement encryption/decryption
- [x] Implement resizing and health checking
- [x] Add a GUI version (in development)

The GUI version will be in development if the Terminal based version funktions as it should!

## License

This project is licensed under the **GNU General Public License v3.0 (GPL-3.0)**.

You are free to use, modify, and distribute this software, **provided that** any modifications are also licensed under GPL-3.0, and proper attribution is given to the original author(s). If you redistribute the software (modified or unmodified), you must make the source code available and include a copy of this license.

**NO WARRANTY**: This software is provided "as-is," without any warranty of any kind, express or implied, including but not limited to the warranties of merchantability or fitness for a particular purpose. In no event shall the authors or copyright holders be liable for any claim, damages, or other liability arising from, out of, or in connection with the software or the use or other dealings in the software.

For full details of the license, see the [LICENSE](./LICENSE) file.


---

**Known Bug/issues:**

no

**Disclaimer:**  
This tool is under active development. Some features may not be fully implemented or stable. Use with caution, and always test on non-critical drives first!

## Ideas or requests?

send me a mesage, dicussion, request,... for some ideas to improve my tool!

## Do you like my tool?

if you like my tool, please give it a star

---
```text
							    kc'.      .,cx
                            0c.             ....;dK
                         X'                ..     'kW
                        Wc   ..        ..''..       oW
                        WW; :k:lx0,   ;XOc;ldXd      .WW
                        WWc ,k,.,OxlooxXx'..;Kx      .WW
						WWx .cxxkOOO0KKKK0Okxkl       xW
						WWX  ;doodxxxxxxddxkkkx'  .,,. ;KW
					  WWKo. ;KXKOkxxxkkOKXNWWMMM0'       'xNW
                   WXx;. .xWMMMMWWNNWWWMMMMMMMMMMMk         'o0W
                 Wd'    'NWWMMMMMWWWMMMMMWWWWNNNXXX0,           :0W
               WX.    .dNWMMMMMMMMMMMMMMMMMMMMMMWWNNNNd.          '0W
             WWx.   .kMMMMMMMMMMWMMMMMMMMMMMMMMMMMMMMMMW:     .     oW
			Wx. .  ,WMMMMMMMMMMWWMMMMMMMMMMMMMMMMMMMMMMMW.    .      lW
          Wx.   .  WMMMMMMMMMMMWWMMMMMMMMMMMMMMMMMMMMMMMM.    .       N
          Wo';:;,..ONMMMMMMMMMMWWMMMMMMMMMMMMMMMMMMMMWNNX.        .. :W
   XXXXXK0kxkOOOOOo,..;oONMMMMMMWMMMMMMMMMMMMMMMMMWWXokOO:        .;dO0
  xxkOOOOOOOOOOOOOOOkl'   .,oXMMMMMMMMMMMMMMMMMMMWXK0oxOOkoc:::cokOOOOk0
  OxkOOOOOOOOOOOOOOOOOOx:....cMMMMMMMMMMMMMMMMMMMKo,,oxOOOOOOOOOOOOOOOOOOOO0K
 XxxkOOOOOOOOOOOOOOOOOOOOkxk00XWMMMMMMMMMMWXOdc'    ;oxOOOOOOOOOOOOOkkkxxddxk
'xddddxxxkkkkkkOOOOOOOOOOkxoc.   ........          .:lxkkOOOkkkxdoloxk0K
            Okkxxdooddddolc:ck0KXXNNNNNNXXKK0OOkdoll;,;:ccccc:cok
```

