# Main Function Documentation

## `main()` — Program Entry Point & TUI Controller

The `main()` function is the central control unit of DriveMgr.
It handles command‑line arguments, configuration overrides, theme initialization, TUI setup, menu rendering, and dispatching user actions.

The function begins by switching the terminal into the alternate screen buffer:

    ```cpp
    std::cout << "\033[?1049h";
    ```

This allows the program to draw a full‑screen TUI without overwriting the user’s normal terminal content.

------------------------------------------------------------
## Theme Initialization

Before processing any input, the program loads the theme colors from the config file:

    ```cpp
    Color_theme_handler();
    ```

This ensures that all UI elements use the correct color scheme.

------------------------------------------------------------
## Command‑Line Argument Parsing (Immediate Actions)

The first loop scans all command‑line arguments and handles flags that should immediately exit the program or modify global behavior:

    ```cpp
    for (int i = 1; i < argc; ++i) {
        std::string a(argv[i]);
    ```

### Dry‑run flag

    ```cpp
        if (a == "--dry-run" || a == "-n") {
            g_dry_run = true;
            continue;
        }
    ```

### Disable color output

    ```cpp
        if (a == "--no-color" || a == "--no_color" || a == "-C") {
            g_no_color = true;
            continue;
        }
    ```

### Help and version info

These flags print information and exit immediately:

    ```cpp
        if (a == "--help" || a == "-h") {
            printUsage(argv[0]);
            return 0;
        }

        if (a == "--version" || a == "-v") {
            std::cout << "DriveMgr CLI version: " << VERSION << "\n";
            return 0;
        }
    ```

### Quick‑access operations

These flags execute a specific operation and then exit:

    ```cpp
        if (a == "--logs") {
            log_viewer();
            return 0;
        }

        if (a == "--list-drives") {
            QuickAccess::list_drives();
            return 0;
        }

        if (a == "--format-drive") {
            QuickAccess::foramt_drive();
            return 0;
        }

        if (a == "--encrypt-decrypt") {
            QuickAccess::de_en_crypt();
            return 0;
        }

        if (a == "--resize-drive") {
            QuickAccess::resize_drive();
            return 0;
        }

        if (a == "--check-drive-health") {
            QuickAccess::check_drive_health();
            return 0;
        }

        if (a == "--analyze-disk-space") {
            QuickAccess::analyze_disk_space();
            return 0;
        }

        if (a == "--overwrite-drive-data") {
            QuickAccess::overwrite_drive_data();
            return 0;
        }

        if (a == "--view-metadata") {
            QuickAccess::view_metadata();
            return 0;
        }

        if (a == "--info") {
            QuickAccess::info();
            return 0;
        }

        if (a == "--forensics") {
            QuickAccess::Forensics();
            return 0;
        }

        if (a == "--disk-space-visualizer") {
            QuickAccess::disk_space_visulizer();
            return 0;
        }

        if (a == "--clone-drive") {
            QuickAccess::clone_drive();
            return 0;
        }
    }
    ```

This allows DriveMgr to be used as a CLI tool without launching the TUI.

------------------------------------------------------------
## Dry‑Run Priority System (Config vs Flags)

The program loads the config file:

    ```cpp
    CONFIG_VALUES cfg = config_handler();
    bool dry_run_mode = cfg.DRY_RUN_MODE;
    ```

If the config file enables dry‑run mode, it is applied:

    ```cpp
    if (dry_run_mode == true) {
        g_dry_run = true;
    }
    ```

Then the program checks if the user explicitly passed `--dry-run`:

    ```cpp
    bool flag_dry_run = false;
    bool flag_dry_run_set = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--dry-run" || arg == "-n") {
            flag_dry_run = true;
            flag_dry_run_set = true;
        }
    }
    ```

If the flag was set, it overrides the config:

    ```cpp
    if (flag_dry_run_set) {
        g_dry_run = true;
    }
    ```

This ensures command‑line flags always have higher priority than config values.

------------------------------------------------------------
## TUI Initialization

The terminal is switched into raw mode to allow single‑key input:

    ```cpp
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    ```

------------------------------------------------------------
## Main Menu Loop

The program enters its main loop:

    ```cpp
    bool running = true;
    while (running == true) {
    ```

A list of menu items is created:

    ```cpp
        std::vector<std::pair<int, std::string>> menuItems = {
            {1, "List Drives"}, {2, "Format Drive"}, {3, "Encrypt/Decrypt Drive (AES-256)"},
            {4, "Resize Drive"}, {5, "Check Drive Health"}, {6, "Analyze Disk Space"},
            {7, "Overwrite Drive Data"}, {8, "View Drive Metadata"}, {9, "View Info/help"},
            {10, "Mount/Unmount/Restore (ISO/Drives/USB)"}, {11, "Forensic Analysis/Disk Image"},
            {12, "Disk Space Visualizer (Beta)"}, {13, "Log viewer"}, {14, "Clone a Drive"},
            {15, "Config Editor"}, {16, "Benchmark"}, {17, "Fingerprint Drive"}, {0, "Exit"}
        };

        // enable raw mode for single-key reading;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ```

------------------------------------------------------------
## Menu Rendering

Inside the inner loop, the screen is cleared and the menu is drawn:

    ```cpp
        int selected = 0;
        while (true) {
            std::string clear = runTerminal("clear");
            std::cout << clear;
            std::cout << "Use Up/Down arrows and Enter to select an option.\n\n";
            std::cout << THEME_COLOR << "┌─────────────────────────────────────────────────┐\n" << RESET;
            std::cout << THEME_COLOR << "│" << RESET << BOLD << "              DRIVE MANAGEMENT UTILITY           " << RESET << THEME_COLOR << "│\n" << RESET;
            std::cout << THEME_COLOR << "├─────────────────────────────────────────────────┤\n" << RESET;
            for (size_t i = 0; i < menuItems.size(); ++i) {
                // Print left border
                std::cout << THEME_COLOR << "│ " << RESET;

                // Build inner content with fixed width
                std::ostringstream inner;
                inner << std::setw(2) << menuItems[i].first << ". " << std::left << std::setw(43) << menuItems[i].second;
                std::string innerStr = inner.str();

                if (menuItems[i].first == 0) {
                    innerStr = CYAN + innerStr + RESET;
                }

                // Apply inverse only to inner content
                if ((int)i == selected) std::cout << INVERSE;
                std::cout << innerStr;
                if ((int)i == selected) std::cout << RESET;

                // Print right border and newline
                std::cout << THEME_COLOR << " │\n" << RESET;
            }
            std::cout  << THEME_COLOR << "└─────────────────────────────────────────────────┘\n" << RESET;
    ```

Arrow keys update the selected index, Enter confirms the selection, and `q`/`Q` provides a quick exit:

    ```cpp
            char c = 0;
            if (read(STDIN_FILENO, &c, 1) <= 0) continue;
            if (c == '\x1b') { // escape sequence
                char seq[2];
                if (read(STDIN_FILENO, &seq, 2) == 2) {
                    if (seq[1] == 'A') { // up
                        selected = (selected - 1 + (int)menuItems.size()) % (int)menuItems.size();
                    } else if (seq[1] == 'B') { // down
                        selected = (selected + 1) % (int)menuItems.size();
                    }
                }
            } else if (c == '\n' || c == '\r') {
                break; // selection made
            } else if (c == 'q' || c == 'Q') {
                // allow quick quit
                selected = (int)menuItems.size() - 1; // Exit item index
                break;
            }
        }
    ```

------------------------------------------------------------
## Menu Action Dispatch

After a selection is made, the terminal is restored:

    ```cpp
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    ```

The selected menu item is then mapped to an action using a `switch` statement:

    ```cpp
        int menuinput = menuItems[selected].first;
        switch (static_cast<MenuOptionsMain>(menuinput)) {
            case LISTDRIVES: {
                listDrives(false);
                std::cout << "\nPress '1' to return, '2' for advanced listing, or '3' to exit:\n";
                int menuques2;
                std::cin >> menuques2;
                if (menuques2 == 1) continue;
                else if (menuques2 == 2) listpartisions();
                else if (menuques2 == 3) running = false;
                break;
            }
            case FORMATDRIVE: {
                checkRoot();
                formatDrive();
                MenuQues(running);
                break;
            }
            case ENCRYPTDECRYPTDRIVE: {
                checkRoot();
                DeEncrypting::main();
                MenuQues(running);
                break;
            }
            case RESIZEDRIVE: {
                checkRoot();
                resizeDrive();
                MenuQues(running);
                break;
            }
            case CHECKDRIVEHEALTH:{
                checkRoot();
                checkDriveHealth();
                MenuQues(running);
                break;
            }
            case ANALYZEDISKSPACE: {
                analyDiskSpace();
                MenuQues(running);
                break;
            }
            case OVERWRITEDRIVEDATA: {
                checkRoot();
                std::cout << "[Warning] This function will overwrite the entire data to zeros. Proceed? (y/n)\n";
                char zerodriveinput;
                std::cin >> zerodriveinput;
                if (zerodriveinput == 'y' || zerodriveinput == 'Y') OverwriteDriveData();
                else std::cout << "[Info] Operation cancelled\n";
                MenuQues(running);
                break;
            }
            case VIEWMETADATA: {
                checkRootMetadata();
                MetadataReader::mainReader();
                MenuQues(running);
                break;
            }
            case VIEWINFO: {
                Info();
                MenuQues(running);
                break;
            }
            case MOUNTUNMOUNT: {
                checkRoot();
                MountUtility::mainMountUtil();
                MenuQues(running);
                break;
            }
            case FORENSIC: {
                checkRoot();
                ForensicAnalysis::mainForensic(running);
                MenuQues(running);
                break;
            }
            case DISKSPACEVIRTULIZER: {
                DSV::DSVmain();
                MenuQues(running);
                break; 
            }
            case EXITPROGRAM: {
                running = false;
                break;
            }
            case LOGVIEW: {
                log_viewer();
                MenuQues(running);
                break;
            }
            case CLONEDRIVE: {
                checkRoot();
                Clone::mainClone();
                MenuQues(running);
                break;
            }
            case CONFIG: {
                config_editor();
                MenuQues(running);
                break;
            }
            case BENCHMAKR: {
                checkRoot();
                Benchmark_main();
                MenuQues(running);
                break;
            }
            case FINGERPRINT: {
                checkRootMetadata();
                DriveFingerprinting::fingerprinting_main();
                MenuQues(running);
                break;
            }
            default: {
                std::cerr << RED << "[Error] Invalid selection\n" << RESET;
                break;
            }
        }
    }
    ```

------------------------------------------------------------
## Program Exit

When the main loop ends, the alternate screen buffer is disabled and the program exits cleanly:

    ```cpp
    std::cout << "\033[?1049l";
    return 0;
    ```
