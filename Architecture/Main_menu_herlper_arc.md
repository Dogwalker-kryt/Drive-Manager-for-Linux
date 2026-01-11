# Info(), printUsage(), MenuQues(), Root‑Check Utilities, MenuOptionsMain, and QuickAccess Documentation

---

## `Info()`

```cpp
void Info() {
    std::cout << "\n┌──────────" << BOLD << " Info " << RESET << "──────────\n";
    std::cout << "│ Welcome to Drive Manager — a program for Linux to view and operate your storage devices.\n"; 
    std::cout << "│ Warning! You should know the basics about drives so you don't lose any data.\n";
    std::cout << "│ If you find problems or have ideas, visit the GitHub page and open an issue.\n";
    std::cout << "│ Other info:\n";
    std::cout << "│ Version: " << VERSION << "\n";
    std::cout << "│ Github: https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux\n";
    std::cout << "│ Author: Dogwalker-kryt\n";
    std::cout << "└───────────────────────────\n";
}
```

### Overview
Displays general program information, including version, author, and GitHub link.  
Used when the user selects the “Info” option from the main menu.

---

## `printUsage()`

```cpp
static void printUsage(const char* progname) {
    std::cout << "Usage: " << progname << " [options]\n";
    std::cout << BOLD << "Options:\n" << RESET;
    std::cout << "  --version, -v     Print program version and exit\n";
    std::cout << "  --help, -h        Show this help and exit\n";
    std::cout << "  -n, --dry-run     Do not perform destructive operations\n";
    std::cout << "  -C, --no-color     Disable colors (may affect the main menu)\n";
    std::cout << "  --operation-name   Goes directly to a specific operation without menu\n";
}
```

### Overview
Prints command‑line usage instructions.  
Supports flags for version, help, dry‑run mode, disabling colors, and direct operation execution.

---

## `MenuQues()`

```cpp
void MenuQues(bool& running) {   
    std::cout << BOLD <<"\nPress '1' for returning to the main menu, '2' to exit:\n" << RESET;
    int menuques;
    std::cin >> menuques;
    if (menuques == 1) {
        running = true;
    } else if (menuques == 2) {
        running = false;
    } else {
        std::cout << RED << "[ERROR] Wrong input\n" << RESET;
        running = true; 
    }
}
```

### Overview
Simple post‑operation prompt that lets the user:

- return to the main menu  
- exit the program  

Invalid input defaults to returning to the menu.

---

## Root‑Check Utilities

### `isRoot()`

```cpp
bool isRoot() {
    return (getuid() == 0);
}
```

Checks whether the program is running with root privileges.

---

### `checkRoot()`

```cpp
void checkRoot() {
    if (!isRoot()) {
        std::cerr << "[ERROR] This Function requires root. Please run with 'sudo'.\n";
        Logger::log("[ERROR] Attempted to run without root privileges -> checkRoot()");
        exit(EXIT_FAILURE);
    }
}
```

### Overview
Hard requirement for operations that modify drives.  
If not root:

- prints error  
- logs attempt  
- terminates program  

---

### `checkRootMetadata()`

```cpp
void checkRootMetadata() {
    if (!isRoot()) {
        std::cerr << YELLOW << "[WARNING] Running without root may limit functionality. For full access, please run with 'sudo'.\n" << RESET;
        Logger::log("[WARNING] Running without root privileges -> checkRootMetadata()");
    }
}
```

### Overview
Soft warning version of `checkRoot()`.  
Used for metadata reading, which can run without root but may be incomplete.

---

## `MenuOptionsMain` Enum

```cpp
enum MenuOptionsMain {
    EXITPROGRAM = 0, LISTDRIVES = 1, FORMATDRIVE = 2, ENCRYPTDECRYPTDRIVE = 3, RESIZEDRIVE = 4, 
    CHECKDRIVEHEALTH = 5, ANALYZEDISKSPACE = 6, OVERWRITEDRIVEDATA = 7, VIEWMETADATA = 8, VIEWINFO = 9,
    MOUNTUNMOUNT = 10, FORENSIC = 11, DISKSPACEVIRTULIZER = 12, LOGVIEW = 13, CLONEDRIVE = 14, CONFIG = 15, BENCHMAKR = 16, FINGERPRINT = 17
};
```

### Overview
Defines all main menu options numerically.  
Used by the main menu switch‑case dispatcher.

---

## `QuickAccess` Class

```cpp
class QuickAccess {
public:
    static void list_drives() {
        listDrives(false);
    }

    static void foramt_drive() {
        checkRoot();
        formatDrive();
    }

    static void de_en_crypt() {
        checkRoot();
        DeEncrypting::main();
    }

    static void resize_drive() {
        checkRoot();
        resizeDrive();
    }

    static void check_drive_health() {
        checkRoot();
        checkDriveHealth();
    }

    static void analyze_disk_space() {
        analyDiskSpace();
    }

    static void overwrite_drive_data() {
        checkRoot();
        OverwriteDriveData();
    }

    static void view_metadata() {
        checkRootMetadata();
        MetadataReader::mainReader();
    }

    static void info() {
        Info();
    }

    static void Forensics() {
        bool no = false;
        checkRoot();
        ForensicAnalysis::mainForensic(no);
    }

    static void disk_space_visulizer() {
        checkRoot();
        DSV::DSVmain();
    }

    static void clone_drive() {
        checkRoot();
        Clone::mainClone();
    } 
};
```

### Overview
Provides **direct shortcuts** to all major DriveMgr operations.  
Each method:

- performs required root checks  
- calls the corresponding subsystem  
- keeps the main menu dispatcher clean and readable  

### Purpose
This class acts as a **centralized operation hub**, enabling:

- direct CLI invocation (`--operation-name`)  
- clean main menu switch logic  
- reusable operation calls  

---

