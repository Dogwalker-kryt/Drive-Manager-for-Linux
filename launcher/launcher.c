#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef enum {
    UI_CLI,
    UI_TUI,
    UI_GUI,
    UI_UNKNOWN
} UI_Mode;

typedef enum {
    COMPILE_JIT,
    COMPILE_STATIC,
    COMPILE_UNKNOWN
} Compile_Mode;

typedef struct {
    char UI_mode[16];       // CLI, TUI, GUI
    char compile_mode[16];  // JIT, StatBin
    int root_mode;          // true/false
} Config;


void trim(char *str) {
    char *start = str;
    while (isspace((unsigned char)*start)) start++;
    if (start != str) memmove(str, start, strlen(start) + 1);
    char *end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end)) *end-- = '\0';
}

UI_Mode parse_ui_mode(const char *str) {
    if (strcasecmp(str, "CLI") == 0) return UI_CLI;
    if (strcasecmp(str, "TUI") == 0) return UI_TUI;
    if (strcasecmp(str, "GUI") == 0) return UI_GUI;
    return UI_UNKNOWN;
}

Compile_Mode parse_compile_mode(const char *str) {
    if (strcasecmp(str, "JIT") == 0) return COMPILE_JIT;
    if (strcasecmp(str, "StatBin") == 0) return COMPILE_STATIC;
    return COMPILE_UNKNOWN;
}


int read_config(const char *filename, Config *cfg) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Error opening config file");
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || strlen(line) < 2) continue;

        char key[64], value[64];
        if (sscanf(line, " %63[^=]=%63s", key, value) == 2) {
            trim(key);
            trim(value);

            if (strcmp(key, "UI_mode") == 0) {
                strncpy(cfg->UI_mode, value, sizeof(cfg->UI_mode));
            } else if (strcmp(key, "compile_mode") == 0) {
                strncpy(cfg->compile_mode, value, sizeof(cfg->compile_mode));
            } else if (strcmp(key, "root_mode") == 0) {
                cfg->root_mode = (strcmp(value, "true") == 0);
            }
        }
    }

    fclose(f);
    return 1;
}


void launch_program(const Config *cfg, const char *home) {
    UI_Mode ui = parse_ui_mode(cfg->UI_mode);
    Compile_Mode cm = parse_compile_mode(cfg->compile_mode);

    const char *UI_exec_paths[] = {
        "/.local/share/DriveMgr/bin/bin/DriveMgr_CLI",
        "/.local/share/DriveMgr/bin/binDriveMgr_TUI",
        "/.local/share/DriveMgr/bin/DriveMgr_GUI"
    };

    if (ui == UI_UNKNOWN) {
        fprintf(stderr, "Unknown UI mode: %s\n", cfg->UI_mode);
        return;
    }

    char abs_path[PATH_MAX];
    snprintf(abs_path, sizeof(abs_path), "%s%s", home, UI_exec_paths[ui]);


    if (access(abs_path, X_OK) != 0) {
        fprintf(stderr, "Executable not found or not executable: %s\n", abs_path);
        return;
    }


    if (cm == COMPILE_JIT) {
        printf("Launching JIT build...\n");
    } else if (cm == COMPILE_STATIC) {
        printf("Launching static binary...\n");
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        if (cfg->root_mode) {
            execlp("sudo", "sudo", abs_path, (char *)NULL);
        } else {
            execlp(abs_path, abs_path, (char *)NULL);
        }
        perror("exec failed");
        exit(1);
    } else if (pid > 0) {
        // Parent waits
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork failed");
    }
}


int main() {
    Config cfg = {0};

    char config_path[PATH_MAX];
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : NULL;
    }

    int ok = 0;
    if (home) {
        snprintf(config_path, sizeof(config_path),
                 "%s/.local/share/DriveMgr/data/config.conf", home);
        if (read_config(config_path, &cfg)) ok = 1;
    }

    if (!ok) {
        if (read_config("config.conf", &cfg)) ok = 1;
    }

    if (ok) {
        printf("UI Mode: %s\n", cfg.UI_mode);
        printf("Compile Mode: %s\n", cfg.compile_mode);
        printf("Root Mode: %s\n", cfg.root_mode ? "true" : "false");

        if (home) {
            launch_program(&cfg, home);
        } else {
            fprintf(stderr, "Could not determine home directory.\n");
        }
    } else {
        printf("Failed to read config file.\n");
    }

    return 0;
}
