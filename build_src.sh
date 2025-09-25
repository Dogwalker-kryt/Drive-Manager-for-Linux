#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

# Drive Manager build helper
# Repo: https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux

# Project root is the directory containing this script
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$ROOT_DIR"
BUILD_DIR="$PROJECT_ROOT/bin"
LOCAL_BIN="$HOME/.local/bin"

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
DIR="${HOME}/.var/app/DriveMgr/bin"
if [ -d "$DIR" ]; then
  cd "$DIR"
  if [ -x ./DriveMgr_stable ]; then
    sudo ./DriveMgr_stable
  elif [ -x ./DriveMgr_experi ]; then
    sudo ./DriveMgr_experi
  else
    echo "No DriveMgr binary found in $DIR"
  fi
  cd - >/dev/null
else
  echo "DriveMgr not installed in $DIR"
fi'

if [ "$DRY_RUN" -eq 1 ]; then
    echo "+ create launcher $LAUNCHER"
    echo "$LAUNCHER_CONTENT"
else
    printf "%s\n" "$LAUNCHER_CONTENT" > "$LAUNCHER"
    chmod +x "$LAUNCHER"
fi

APP_DIR="$HOME/.var/app/DriveMgr"
ensure_dir "$APP_DIR/bin"
if [ -d "$BUILD_DIR" ]; then
    run_cmd cp -u "$BUILD_DIR"/* "$APP_DIR/bin/" 2>/dev/null || warn "No built executables to copy"
fi

run_cmd touch "$APP_DIR/log.dat" "$APP_DIR/keys.bin"
info "Done. Binaries: $BUILD_DIR, App dir: $APP_DIR/bin, launcher: $LAUNCHER"

exit 0
