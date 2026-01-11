## Config Functionality Overview

This section explains how the configuration system works, how the config file is located, and how its values are parsed into the `CONFIG_VALUES` struct.

------------------------------------------------------------

## `config_path()` — Locating the Config File

The function begins by determining which user is running the program.
It checks both the `SUDO_USER` and `USER` environment variables:

    ```cpp
    const char* sudo_user = std::getenv("SUDO_USER");
    const char* user_env = std::getenv("USER");
    const char* username = sudo_user ? sudo_user : user_env;
    ```

- `SUDO_USER` is used when the program is executed with `sudo`.
- `USER` is used for normal execution.
- The `username` variable selects whichever one is available.

Next, the function validates that a username was found and attempts to retrieve the user’s home directory:

    ```cpp
    if (!username) {
        std::cerr << RED << "[Config Error] Could not determine username.\n" << RESET;
        return "";
    }

    struct passwd* pw = getpwnam(username);
    if (!pw) {
        std::cerr << RED << "[Config Error] Failed to get home directory for user: " << username << "\n" << RESET;
        return "";
    }
    ```

If both checks succeed, the function constructs the full path to the configuration file:

    ```cpp
    std::string homeDir = pw->pw_dir;
    return homeDir + "/.local/share/DriveMgr/data/config.conf";
    // If you want to change the config file location, this is the place to do it.
    ```

------------------------------------------------------------

## `CONFIG_VALUES` — Configuration Structure

This struct stores all values that can be read from the configuration file:

    ```cpp
    struct CONFIG_VALUES {
        std::string UI_MODE;
        std::string COMPILE_MODE;
        std::string ROOT_MODE;
        std::string THEME_COLOR_MODE;
        std::string SELECTION_COLOR_MODE;
        bool DRY_RUN_MODE;
    };
    ```

Each field corresponds to a key in the config file.

------------------------------------------------------------

## `config_handler()` — Parsing the Config File

The handler begins by initializing an empty `CONFIG_VALUES` struct and retrieving the config file path:

    ```cpp
    CONFIG_VALUES config_handler() {
        CONFIG_VALUES cfg{};

        std::string conf_file = config_path();
        if (conf_file.empty()) {
            return cfg;
        }
    ```

It then attempts to open the file:

    ```cpp
    std::ifstream config_file(conf_file);
    if (!config_file.is_open()) {
        Logger::log("[Config_handler ERROR] Cannot open config file");
        std::cerr << RED << "[Config_handler ERROR] Cannot open config file\n" << RESET;
        return cfg;
    }
    ```

If the file opens successfully, each line is processed:

    ```cpp
    std::string line;
    while (std::getline(config_file, line)) {

        // Ignore empty lines and comments
        if (line.empty() || line[0] == '#')
            continue;

        // Only process lines containing '='
        size_t pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);

        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
    ```

Finally, the values are assigned to the struct:

    ```cpp
        if (key == "UI_MODE") cfg.UI_MODE = value;
        else if (key == "COMPILE_MODE") cfg.COMPILE_MODE = value;
        else if (key == "ROOT_MODE") cfg.ROOT_MODE = value;
        else if (key == "COLOR_THEME") cfg.THEME_COLOR_MODE = value;
        else if (key == "SELECTION_COLOR") cfg.SELECTION_COLOR_MODE = value;
        else if (key == "DRY_RUN_MODE") {
            std::string v = value;
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            cfg.DRY_RUN_MODE = (v == "true");
        }
    }
    ```

The fully populated struct is then returned:

    ```cpp
    return cfg;
    }
    ```

------------------------------------------------------------

------------------------------------------------------------
## `config_editor()` — Interactive Config Viewer & Editor

The `config_editor()` function allows the user to view the current configuration values and optionally open the config file in an external editor.

It begins by loading the current configuration:

    ```cpp
    CONFIG_VALUES cfg = config_handler();
    ```

The function then prints a small UI box showing all currently loaded config values:

    ```cpp
    std::cout<< "┌─────" << BOLD << " config values " << RESET << "─────┐\n";
    std::cout << "│ UI mode: " << cfg.UI_MODE << "\n";
    std::cout << "│ Compile mode: " << cfg.COMPILE_MODE << "\n";
    std::cout << "│ Root mode: " << cfg.ROOT_MODE << "\n";
    std::cout << "│ Theme Color: " << cfg.THEME_COLOR_MODE << "\n";
    std::cout << "│ Selection Color: " << cfg.SELECTION_COLOR_MODE << "\n";
    std::cout << "└─────────────────────────┘\n";
    ```

After displaying the values, the user is asked whether they want to edit the config file:

    ```cpp
    std::cout << "Do you want to edit the config file? (y/n)\n";
    std::string config_edit;
    std::cin >> config_edit;
    ```

If the user answers `"n"` or provides an empty input, the function exits:

    ```cpp
    if (config_edit == "n" || config_edit.empty()) {
        return;
    }
    ```

If the user answers `"y"`, the function attempts to locate the external editor **Lume**.

First, it determines the username (same logic as in `config_path()`):

    ```cpp
    const char* sudo_user = std::getenv("SUDO_USER");
    const char* user_env = std::getenv("USER");
    const char* username = sudo_user ? sudo_user : user_env;
    ```

If the username or home directory cannot be determined, an error is printed and the function returns:

    ```cpp
    if (!username) {
        std::cerr << "[Config Editor Error] Could not determine username.\n";
        return;
    }

    struct passwd* pw = getpwnam(username);
    if (!pw) {
        std::cerr << "[Config Editor Error] Failed to get home directory for user: " << username << "\n";
        return;
    }
    ```

Next, the function constructs the path to the Lume editor:

    ```cpp
    std::string lumePath = homeDir + "/.local/share/DriveMgr/bin/Lume/Lume";
    ```

If Lume is not found, the user is informed:

    ```cpp
    if (!fileExists(lumePath)) {
        std::cerr << RED << "[Config Editor Error] Lume editor not found at:" << lumePath << "\n" << RESET;
        return;
    }
    ```

If Lume exists, the function prepares the command to open the config file:

    ```cpp
    std::string configPath = homeDir + "/.local/share/DriveMgr/data/config.conf";
    std::string config_editor_cmd = "\"" + lumePath + "\" \"" + configPath + "\"";
    ```

Before launching the editor, the terminal is restored from raw mode and the alternate screen is disabled:

    ```cpp
    std::cout << "\033[?1049l" << std::flush;
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    ```

The Lume editor is then executed:

    ```cpp
    system(config_editor_cmd.c_str());
    ```

After the editor exits, the alternate screen is re-enabled and raw mode is restored:

    ```cpp
    std::cout << "\033[?1049h" << std::flush;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ```

------------------------------------------------------------
## Color Arrays — Available Theme Colors

These two arrays define all supported color names and their corresponding ANSI escape codes:

    ```cpp
    std::string avilable_colores[6] = { "RED", "GREEN", "YELLOW", "BLUE", "MAGENTA", "CYAN"};
    std::string color_codes[6] = {RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN};
    ```

**Important:**  
If new colors are added to the program, they must also be added to both arrays.

------------------------------------------------------------
## `Color_theme_handler()` — Applying Theme Colors

This function reads the theme values from the config file and applies the correct ANSI color codes to the global variables `THEME_COLOR` and `SELECTION_COLOR`.

It begins by loading the config:

    ```cpp
    CONFIG_VALUES cfg = config_handler();
    std::string color_theme_name = cfg.THEME_COLOR_MODE;
    std::string selection_theme_name = cfg.SELECTION_COLOR_MODE;
    ```

The number of available colors is calculated:

    ```cpp
    size_t count = sizeof(avilable_colores) / sizeof(avilable_colores[0]);
    ```

### Theme Color Selection

The default theme color is reset:

    ```cpp
    THEME_COLOR = RESET;
    ```

The function loops through all available colors and compares the config value to the color names:

    ```cpp
    for (size_t i = 0; i < count; i++) {
        if (avilable_colores[i] == color_theme_name) {
            THEME_COLOR = color_codes[i];
            break;
        }
    }
    ```

**Important:**
THEME_COLOR is used in the main function while drawing the main menu.


### Selection Color

The same logic is applied for the selection highlight color:

    ```cpp
    SELECTION_COLOR = RESET;

    for (size_t i = 0; i < count; i++) {
        if (avilable_colores[i] == selection_theme_name) {
            SELECTION_COLOR = color_codes[i];
            break;
        }
    }
    ```

**Important:**
SELECTION_COLOR is used in the listDrive function, with the TUI selector.


This ensures that both theme colors are always valid and fall back to `RESET` if the config contains an unknown value.

------------------------------------------------------------

