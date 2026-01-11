# MountUtility Class Documentation  
Includes: **BurnISOToStorageDevice**, **MountDrive2**, **UnmountDrive2**, **Restore_USB_Drive**, **ExitReturn**, **mainMountUtil**

---

## `BurnISOToStorageDevice()`

```cpp
static void BurnISOToStorageDevice() { 
    try {
        std::string driveName = listDrives(true);

        std::cout << "Are you sure you want to select " << driveName << " for this operation? (y/n)\n";
        char confirmation_burn;
        std::cin >> confirmation_burn;
        if (confirmation_burn != 'y' && confirmation_burn != 'Y') {
            std::cout << "[Info] Operation cancelled\n";
            Logger::log("[INFO] Operation cancelled -> BurnISOToStorageDevice()");
            return;
        }

        std::cout << "\nEnter the path to the iso/img file you want to burn on " << driveName << ":\n";
        std::string isoPath;
        std::cin >> isoPath;
        std::cout << "Are you sure you want to burn " << isoPath << " to " << driveName << "? (y/n)\n";
        char confirmation_burn2;
        std::cin >> confirmation_burn2;
        if (confirmation_burn2 != 'y' && confirmation_burn2 != 'Y') {
            std::cout << "[Info] Operation cancelled\n";
            Logger::log("[INFO] Operation cancelled -> BurnISOToStorageDevice()");
            return;
        }

        char random_confirmationkey[] = {
            'a','A','b','B','c','C','d','D','e','E','f','F','g','G','h','H','i','I','j','J','k','K','l','L','m','M','n','N','o','O','p','P','q','Q','r','R','s','S','t','T','u','U','v','V','w','W','x','X','y','Y','z','Z',
            '0','1','2','3','4','5','6','7','8','9'
        };
        for (int i = 0; i < 10; i++) {
            int randomIndex = rand() % (sizeof(random_confirmationkey) / sizeof(random_confirmationkey[0]));
            std::cout << random_confirmationkey[randomIndex];
        }

        std::cout << "\nPlease enter the confirmationkey to proceed with the operation:\n";
        std::cout << random_confirmationkey << "\n";
        char random_confirmationkey_input[10];
        std::cin >> random_confirmationkey_input;
        if (std::string(random_confirmationkey_input) != std::string(random_confirmationkey)) {
            std::cout << "[Error] Invalid confirmation of the Key or unexpected error\n";
            Logger::log("[ERROR] Invalid confirmation of the Key or unexpected error -> OverwriteData");
            return;
        } else {
            try {
                std::cout << "Proceeding with burning " << isoPath << " to " << driveName << "...\n";
                std::string bruncmd = "sudo dd if=" + isoPath + " of=" + driveName + " bs=4M status=progress && sync";
                std::string brunoutput = runTerminalV2(bruncmd);
                if (brunoutput.find("error") != std::string::npos) {
                    Logger::log("[ERROR] Failed to burn iso/img to drive: " + driveName + " -> BurnISOToStorageDevice()"); 
                    throw std::runtime_error("[Error] Failed to burn iso/img to drive: " + driveName);
                }

                std::cout << "[Success] Successfully burned " << isoPath << " to " << driveName << "\n";
                Logger::log("[INFO] Successfully burned iso/img to drive: " + driveName + " -> BurnISOToStorageDevice()");
            } catch (const std::exception& e) {
                std::cout << e.what() << "\n";
            } 
        }
    } catch (const std::exception& e) {
        std::cout << RED << "[ERROR] Failed to initialize burn iso/img: " << e.what() << RESET << "\n";
        Logger::log("[ERROR] Failed to burn iso/img: " + std::string(e.what()));
        return;
    }
}
```

### Overview
Burns an ISO/IMG file directly onto a selected storage device using `dd`.  
Includes **double confirmation**, **random confirmation key**, and **error logging** to prevent accidental data loss.

---

## `MountDrive2()`

```cpp
static void MountDrive2() {
    try{
        std::string driveName = listDrives(true);

        std::string mountpoint = "sudo mount " + driveName + " /mnt/" + std::filesystem::path(driveName).filename().string();
        std::string mountoutput = runTerminalV2(mountpoint);
        if (mountoutput.find("error") != std::string::npos) {
            std::cout << "[Error] Failed to mount drive: " << mountoutput << "\n";
            Logger::log("[ERROR] Failed to mount drive: " + driveName + " -> MountDrive()");
            return;
        }
    } catch (const std::exception& e) {
        std::cerr << RED << "[ERROR] Failed to initialize mount disk: " << e.what() << RESET << "\n";
        Logger::log("[ERROR] Failed to initialize mount disk: " + std::string(e.what()));
        return;
    }
}
```

### Overview
Mounts a selected drive under `/mnt/<device-name>`.  
Logs errors and handles exceptions.

---

## `UnmountDrive2()`

```cpp
static void UnmountDrive2() {
    try {
        std::string driveName = listDrives(true );

        std::string unmountpoint = "sudo umount " + driveName;
        std::string unmountoutput = runTerminalV2(unmountpoint);
        if (unmountoutput.find("error") != std::string::npos) {
            std::cerr << RED << "[ERROR] Failed to unmount drive: " << unmountoutput << "\n";
            Logger::log("[ERROR] Failed to unmount drive: " + driveName + " -> UnmountDrive()");
            return;
        }
    } catch (const std::exception& e) {
        std::cerr << RED << "[ERROR] Failed to initialize disk unmounting: " << e.what() << "\n";
        Logger::log("[ERROR] Failed to initialize disk unmounting: " + std::string(e.what()));
        return; 
    }
}
```

### Overview
Unmounts a selected drive using `umount`.  
Handles errors and logs failures.

---

## `Restore_USB_Drive()`

```cpp
static void Restore_USB_Drive() {
    std::string restore_device_name = listDrives(true);

    try {
        std::cout << "Are you sure you want to overwrite/clean the ISO/Disk_Image from: " << restore_device_name << " ? [y/N]\n";
        char confirm = 'n';
        std::cin >> confirm;
        if (std::tolower(confirm) != 'y') {
            std::cout << "Restore process aborted\n";
            return;
        }

        std::string unmount_cmd = "sudo umount " + restore_device_name + "* 2>/dev/null || true";
        Terminalexec::execTerminalv2(unmount_cmd.c_str());
        std::string wipefs_cmd = "sudo wipefs -a " + restore_device_name + " 2>&1";
        std::string wipefs_out = Terminalexec::execTerminalv2(wipefs_cmd.c_str());
        std::string dd_cmd = "sudo dd if=/dev/zero of=" + restore_device_name + " bs=1M count=10 status=progress && sudo sync";
        std::string dd_out = Terminalexec::execTerminalv2(dd_cmd.c_str());

        if (dd_out.find("error") != std::string::npos || dd_out.find("failed") != std::string::npos) {
            Logger::log("[ERROR] Failed to overwrite the iso image on the usb -> Restore_USB_Drive()");
            std::cerr << "[Error] Failed to overwrite device: " << restore_device_name << "\n";
            return;
        }

        std::string parted_cmd = "sudo parted -s " + restore_device_name + " mklabel msdos mkpart primary 1MiB 100%";
        std::string parted_out = Terminalexec::execTerminalv2(parted_cmd.c_str());

        std::string partprobe_cmd = "sudo partprobe " + restore_device_name + " 2>&1";
        std::string partprobe_out = Terminalexec::execTerminalv2(partprobe_cmd.c_str());

        std::string partition_path = restore_device_name;
        if (!partition_path.empty() && std::isdigit(partition_path.back())) partition_path += "p1"; else partition_path += "1";
        
        std::string mkfs_cmd = "sudo mkfs.vfat -F32 " + partition_path + " 2>&1";
        std::string mkfs_out = Terminalexec::execTerminalv2(mkfs_cmd.c_str());

        if (parted_out.find("error") != std::string::npos || partprobe_out.find("error") != std::string::npos || mkfs_out.find("error") != std::string::npos) {
            Logger::log("[ERROR] Failed while restoring USB: " + restore_device_name);
            std::cerr << "[Error] One or more steps failed while restoring device. Check output.\n";
            return;
        }

        std::cout << GREEN << "[Success] Your USB should now function as a normal FAT32 drive (partition: " << partition_path << ")\n" << RESET;
        Logger::log("[INFO] Restored USB device " + restore_device_name + " -> formatted " + partition_path);
        return;
    } catch (const std::exception& e) {
        std::cerr << RED << "[ERROR] Failed to initialize usb restore function: " << e.what() << RESET << "\n";
        Logger::log("[ERROR] failed to initialize restore usb function");
        return;
    }
}
```

### Overview
Restores a USB drive that previously had an ISO burned onto it.  
Steps include:

- unmounting  
- wiping filesystem signatures  
- zeroing first 10MB  
- recreating partition table  
- formatting FAT32  

---

## `ExitReturn()`

```cpp
static int ExitReturn(bool& running) {
    running = false;
    return 0;   
}
```

### Overview
Utility to exit mount menu loops.

---

## `mainMountUtil()`

```cpp
static void mainMountUtil() {
    enum MenuOptions {
        Burniso = 1, MountDrive = 2, UnmountDrive = 3, Exit = 0, RESTOREUSB = 4
    };
    std::cout << "\n--------- Mount menu ---------\n";
    std::cout << "1. Burn iso/img to storage device\n";
    std::cout << "2. Mount storage device\n";
    std::cout << "3. Unmount storage device\n";
    std::cout << "4. Restore usb from iso\n";
    std::cout << "0. Rreturn to main menu\n";
    std::cout << "--------------------------------\n";
    int menuinputmount;
    std::cin >> menuinputmount;
    switch (menuinputmount) {
        case Burniso: {
            BurnISOToStorageDevice();
            break;
        }
        case MountDrive: {
            MountDrive2();
            break;
        }
        case UnmountDrive: {
            UnmountDrive2();
            break;
        }
        case RESTOREUSB: {
            Restore_USB_Drive();
            break;
        }
        case Exit: {
            return;
        }
        default: {
            std::cout << "[Error] Invalid selection\n";
            return;
        }
    }
}
```

### Overview
Provides an interactive mount menu with four operations:

1. Burn ISO/IMG  
2. Mount drive  
3. Unmount drive  
4. Restore USB  

Handles user input and dispatches to the appropriate internal function.

---

