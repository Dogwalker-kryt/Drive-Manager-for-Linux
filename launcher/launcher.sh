#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

APP_DATA_DIR="$HOME/.local/share/DriveMgr/data"
APP_BIN_DIR="$HOME/.local/share/DriveMgr/bin"
CONFIG_FILE="$APP_DATA_DIR/config.conf"

info(){ echo "[INFO] $*"; }
err(){ echo "[ERROR] $*" >&2; }

# Default config
UI_MODE="CLI"
COMPILE_MODE="STATBIN"  # JIT or STATBIN
ROOT_MODE="false"

# Read config file if present
if [ -f "$CONFIG_FILE" ]; then
  while IFS='=' read -r key val; do
    # trim
    key="$(echo "$key" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')"
    val="$(echo "${val:-}" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')"
    # skip comments and empty
    case "$key" in
      ''|\#*) continue ;;
    esac
    case "$key" in
      UI_mode) UI_MODE="$val" ;;
      compile_mode) COMPILE_MODE="$val" ;;
      root_mode) ROOT_MODE="$val" ;;
    esac
  done < "$CONFIG_FILE"
else
  info "Config file not found at $CONFIG_FILE — using defaults"
fi

# Normalize
UI_MODE_UPPER="$(echo "$UI_MODE" | tr '[:lower:]' '[:upper:]')"
COMPILE_MODE_UPPER="$(echo "$COMPILE_MODE" | tr '[:lower:]' '[:upper:]')"
ROOT_MODE_LOWER="$(echo "$ROOT_MODE" | tr '[:upper:]' '[:lower:]')"

use_sudo=""
if [ "$ROOT_MODE_LOWER" = "true" ]; then
  use_sudo="sudo"
fi

run_binary_if_exists(){
  # args: list of candidate basenames
  for name in "$@"; do
    path="$APP_BIN_DIR/$name"
    if [ -x "$path" ]; then
      info "Running $path"
      exec $use_sudo "$path"
      return 0
    fi
  done
  return 1
}

compile_and_run(){
  local srcpath="$1"; shift
  if [ ! -f "$srcpath" ]; then
    err "Source not found: $srcpath"
    return 1
  fi
  tmpout="/tmp/drivemgr_$(basename "$srcpath" .cpp)_$$"
  info "Compiling $srcpath -> $tmpout"
  if command -v g++ >/dev/null 2>&1; then
    if g++ -std=c++17 -O2 "$srcpath" -o "$tmpout" -lssl -lcrypto; then
      info "Compilation succeeded, running $tmpout"
      exec $use_sudo "$tmpout"
      return 0
    else
      err "Compilation failed for $srcpath"
      return 2
    fi
  else
    err "g++ not found; cannot JIT compile"
    return 3
  fi
}

case "$COMPILE_MODE_UPPER" in
  JIT)
    case "$UI_MODE_UPPER" in
      CLI)
        SRC="$APP_BIN_DIR/src/DriveMgr_CLI.cpp"
        compile_and_run "$SRC"
        ;; 
      TUI)
        SRC="$APP_BIN_DIR/src/DriveMgr_TUI.cpp"
        compile_and_run "$SRC"
        ;;
      GUI)
        SRC="$APP_BIN_DIR/src/DriveMgr_GUI.cpp"
        compile_and_run "$SRC"
        ;;
      *)
        err "Unknown UI mode: $UI_MODE"
        exit 1
        ;;
    esac
    ;;
  STATBIN)
    case "$UI_MODE_UPPER" in
      CLI)
        # try several known CLI binary names
        run_binary_if_exists DriveMgr_CLI DriveMgr_experi DriveMgr_stable || { err "No CLI binary found in $APP_BIN_DIR"; exit 1; }
        ;;
      TUI)
        run_binary_if_exists DriveMgr_TUI DriveMgr_tui DriveMgr_experi || { err "No TUI binary found in $APP_BIN_DIR"; exit 1; }
        ;;
      GUI)
        run_binary_if_exists DriveMgr_GUI DriveMgr_gui DriveMgr_experi || { err "No GUI binary found in $APP_BIN_DIR"; exit 1; }
        ;;
      *)
        err "Unknown UI mode: $UI_MODE"
        exit 1
        ;;
    esac
    ;;
  *)
    err "Unknown compile_mode: $COMPILE_MODE"
    exit 1
    ;;
esac

exit 0
#!/usr/bin/env bash
# read the config file
config_path="~/.local/share/DriveMgr/data/config.conf"
if [ -f "$config_path" ]; then
    # shellcheck source=/dev/null
    source "$config_path"
else
    echo "[Error] Config file not found at $config_path"
    exit 1
fi


