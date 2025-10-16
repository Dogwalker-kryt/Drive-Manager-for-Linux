#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

# Drive Manager build helper
# Repo: https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux

# Project root is the directory containing this script
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$ROOT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/bin"
LOCAL_BIN="$HOME/.local/bin"

# Application install paths
APP_ROOT="$HOME/.local/share/DriveMgr"
APP_BIN_DIR="$APP_ROOT/bin"
APP_LAUNCHER_DIR="$APP_ROOT/launcher"
APP_SRC_DIR="$APP_ROOT/src"
APP_DATA_DIR="$APP_ROOT/data"

# Defaults
DRY_RUN=0
DO_INSTALL=1
TARGETS=(cli gui)

usage(){
    cat <<USAGE
Usage: $(basename "$0") [options]
Options:
  --dry-run        Print commands instead of running them
  --no-install     Do not attempt to install missing packages
  --targets=LIST   Comma-separated targets: cli,gui (default: cli,gui)
  -h, --help       Show this help
USAGE
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --dry-run) DRY_RUN=1; shift;;
        --no-install) DO_INSTALL=0; shift;;
        --targets=*) IFS=',' read -r -a TARGETS <<< "${1#*=}"; shift;;
        -h|--help) usage; exit 0;;
        *) echo "Unknown arg: $1"; usage; exit 1;;
    esac
done

run_cmd(){
    if [ "$DRY_RUN" -eq 1 ]; then
        echo "+ $*"
    else
        eval "$@"
    fi
}

detect_pkg_manager() {
    if command -v apt-get >/dev/null 2>&1; then
        echo "apt"
    elif command -v dnf >/dev/null 2>&1; then
        echo "dnf"
    elif command -v pacman >/dev/null 2>&1; then
        echo "pacman"
    elif command -v zypper >/dev/null 2>&1; then
        echo "zypper"
    else
        echo "unknown"
    fi
}

ensure_dir(){ local d="$1"; [ -d "$d" ] || run_cmd mkdir -p "$d"; }
check_cmd(){ command -v "$1" >/dev/null 2>&1; }
warn(){ echo "[WARN] $*" >&2; }
info(){ echo "[INFO] $*"; }
err(){ echo "[ERROR] $*" >&2; }

PKG_MANAGER="$(detect_pkg_manager)"
info "Detected package manager: $PKG_MANAGER"

missing=()
check_and_add(){ local cmd="$1" pkg="$2"; check_cmd "$cmd" || missing+=("$pkg"); }
check_and_add g++ g++
check_and_add rustc rustc
check_and_add cargo cargo
check_and_add openssl openssl
check_and_add smartctl smartmontools

if [ "${#missing[@]}" -ne 0 ]; then
    echo "Missing: ${missing[*]}"
    if [ "$DO_INSTALL" -eq 1 ]; then
        read -r -p "Attempt to install them (requires sudo)? [y/N] " choice
        if [ "${choice,,}" = "y" ]; then
            case "$PKG_MANAGER" in
                apt) run_cmd sudo apt update; run_cmd sudo apt install -y ${missing[*]} ;;
                dnf) run_cmd sudo dnf install -y ${missing[*]} ;;
                pacman) run_cmd sudo pacman -Sy --noconfirm ${missing[*]} ;;
                zypper) run_cmd sudo zypper install -y ${missing[*]} ;;
                *) err "Pkg manager unknown. Please install: ${missing[*]}"; exit 1 ;;
            esac
        else
            warn "Skipping installation of dependencies"
        fi
    else
        warn "--no-install given; skipping dependency installs"
    fi
fi

ensure_dir "$BUILD_DIR"

build_cli(){
    if [ ! -d "$PROJECT_ROOT/DriveMgr_CLI/src" ]; then warn "CLI source missing"; return; fi
    pushd "$PROJECT_ROOT/DriveMgr_CLI/src" >/dev/null
    for src in *.cpp; do
        [ -f "$src" ] || continue
        out="${src%.cpp}"
        info "g++ -std=c++17 -O2 -I.. $src -o $BUILD_DIR/$out -lssl -lcrypto"
        run_cmd g++ -std=c++17 -O2 -I.. "$src" -o "$BUILD_DIR/$out" -lssl -lcrypto || { err "compile failed: $src"; popd >/dev/null; exit 1; }
    done
    popd >/dev/null
}

build_gui(){
    if ! check_cmd cargo || [ ! -d "$PROJECT_ROOT/DriveMgr_GUI" ]; then warn "Skipping Rust GUI"; return; fi
    pushd "$PROJECT_ROOT/DriveMgr_GUI" >/dev/null
    info "cargo build --release"
    run_cmd cargo build --release || { err "cargo build failed"; popd >/dev/null; exit 1; }
    for b in target/release/*; do
        [ -x "$b" ] && [ -f "$b" ] && run_cmd cp "$b" "$BUILD_DIR/" || true
    done
    popd >/dev/null
}

for t in "${TARGETS[@]}"; do
    case "$t" in
        cli) build_cli ;;
        gui) build_gui ;;
        *) warn "Unknown target: $t" ;;
    esac
done

info "Installing launcher to $LOCAL_BIN"
ensure_dir "$LOCAL_BIN"
LAUNCHER="$LOCAL_BIN/dmgrctl"
LAUNCHER_CONTENT='#!/usr/bin/env bash
APP_BIN_DIR="$HOME/.local/share/DriveMgr/bin"
APP_LAUNCHER_DIR="$HOME/.local/share/DriveMgr/launcher"
LAUNCHER_SCRIPT="$APP_LAUNCHER_DIR/launcher.sh"
if [ -f "$LAUNCHER_SCRIPT" ]; then
    exec "$LAUNCHER_SCRIPT" "$@"
fi
if [ -d "$APP_BIN_DIR" ]; then
    cd "$APP_BIN_DIR"
    if [ -x ./DriveMgr_stable ]; then
        sudo ./DriveMgr_stable "$@"
    elif [ -x ./DriveMgr_experi ]; then
        sudo ./DriveMgr_experi "$@"
    else
        echo "No DriveMgr binary found in $APP_BIN_DIR"
    fi
    cd - >/dev/null
else
    echo "DriveMgr not installed in $APP_BIN_DIR"
fi'


if [ "$DRY_RUN" -eq 1 ]; then
    echo "+ create launcher $LAUNCHER"
    echo "$LAUNCHER_CONTENT"
else
    printf "%s\n" "$LAUNCHER_CONTENT" > "$LAUNCHER"
    chmod +x "$LAUNCHER"
fi


# Ensure application directories for layout B
ensure_dir "$APP_ROOT"
ensure_dir "$APP_BIN_DIR"
ensure_dir "$APP_LAUNCHER_DIR"
ensure_dir "$APP_SRC_DIR"
ensure_dir "$APP_DATA_DIR"

# Ensure local bin exists and set permissions
ensure_dir "$LOCAL_BIN"
run_cmd chmod 755 "$LOCAL_BIN" || warn "Failed to chmod $LOCAL_BIN"

# Set secure permissions for app dirs
run_cmd chmod 700 "$APP_DATA_DIR" || warn "Failed to chmod $APP_DATA_DIR"
run_cmd chmod 755 "$APP_BIN_DIR" || warn "Failed to chmod $APP_BIN_DIR"
run_cmd chmod 755 "$APP_LAUNCHER_DIR" || warn "Failed to chmod $APP_LAUNCHER_DIR"

if [ -d "$BUILD_DIR" ]; then
    # Copy built executables to application bin dir
    run_cmd cp -u "$BUILD_DIR"/* "$APP_BIN_DIR/" 2>/dev/null || warn "No built executables to copy"
    # Ensure copied files are executable
    for f in "$APP_BIN_DIR"/*; do
        [ -f "$f" ] || continue
        run_cmd chmod +x "$f" || true
    done
fi

# If the repository contains a launcher.sh at project root, install it into the launcher folder
if [ -f "$PROJECT_ROOT/launcher.sh" ]; then
    info "Installing repository launcher.sh to $APP_LAUNCHER_DIR/launcher.sh"
    run_cmd cp "$PROJECT_ROOT/launcher.sh" "$APP_LAUNCHER_DIR/launcher.sh"
    run_cmd chmod +x "$APP_LAUNCHER_DIR/launcher.sh"
fi

# Also copy CLI source and include files into the installed src folder so JIT can use them
if [ -d "$PROJECT_ROOT/DriveMgr_CLI/src" ]; then
    info "Copying CLI source files to $APP_SRC_DIR"
    run_cmd cp -u "$PROJECT_ROOT/DriveMgr_CLI/src/"*.cpp "$APP_SRC_DIR/" 2>/dev/null || true
fi
if [ -d "$PROJECT_ROOT/DriveMgr_CLI/include" ]; then
    info "Copying CLI include files to $APP_SRC_DIR/include"
    ensure_dir "$APP_SRC_DIR/include"
    run_cmd cp -u "$PROJECT_ROOT/DriveMgr_CLI/include/"* "$APP_SRC_DIR/include/" 2>/dev/null || true
fi

# If config file missing, create a sensible default
DEFAULT_CONFIG="$APP_DATA_DIR/config.conf"
if [ ! -f "$DEFAULT_CONFIG" ]; then
    info "Creating default config at $DEFAULT_CONFIG"
    if [ "$DRY_RUN" -eq 1 ]; then
        echo "+ create $DEFAULT_CONFIG"
    else
        cat > "$DEFAULT_CONFIG" <<EOF
# DriveMgr default config
UI_mode=CLI
compile_mode=StatBin
root_mode=false
EOF
        chmod 600 "$DEFAULT_CONFIG" || warn "Failed to chmod $DEFAULT_CONFIG"
    fi
fi

# Create data files and apply secure permissions
run_cmd touch "$APP_DATA_DIR/log.dat" || warn "Failed to touch log.dat"
run_cmd touch "$APP_DATA_DIR/keys.bin" || warn "Failed to touch keys.bin"
run_cmd chmod 600 "$APP_DATA_DIR/keys.bin" || warn "Failed to chmod keys.bin"
run_cmd chmod 644 "$APP_DATA_DIR/log.dat" || warn "Failed to chmod log.dat"

info "Done. Binaries: $APP_BIN_DIR, App data: $APP_DATA_DIR, launcher: $LAUNCHER"

exit 0
