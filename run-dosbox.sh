#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
APP_NAME="${OUTPUT_EXE:-program.exe}"
APP_PATH="${APP_PATH:-$SCRIPT_DIR/bin/$APP_NAME}"

find_dosbox() {
    local candidate
    for candidate in dosbox dosbox-x; do
        if command -v "$candidate" >/dev/null 2>&1; then
            echo "$(command -v "$candidate")"
            return 0
        fi
    done

    local common_paths=(
        "/opt/homebrew/bin/dosbox"
        "/usr/local/bin/dosbox"
        "/usr/bin/dosbox"
        "$HOME/.local/bin/dosbox"
        "/Applications/DOSBox.app/Contents/MacOS/DOSBox"
    )

    for candidate in "${common_paths[@]}"; do
        if [[ -x "$candidate" ]]; then
            echo "$candidate"
            return 0
        fi
    done

    return 1
}

ensure_build() {
    if [[ -f "$APP_PATH" ]]; then
        return 0
    fi

    if command -v docker >/dev/null 2>&1; then
        echo "Binary not found; building with Docker..."
        (cd "$SCRIPT_DIR" && ./build-docker.sh)
        return 0
    fi

    if command -v wcl >/dev/null 2>&1; then
        echo "Binary not found; building with local Open Watcom..."
        (cd "$SCRIPT_DIR" && ./build.sh)
        return 0
    fi

    echo "Neither Docker nor Open Watcom is available." >&2
    echo "Install DOSBox and a compiler, or run the build script first." >&2
    exit 1
}

if [[ ! -f "$APP_PATH" ]]; then
    ensure_build
fi

DOSBOX_BIN="$(find_dosbox || true)"
if [[ -z "${DOSBOX_BIN:-}" ]]; then
    echo "DOSBox was not found in PATH or common install locations." >&2
    echo "Install it with: brew install dosbox" >&2
    exit 1
fi

TMP_CONF="$(mktemp "${TMPDIR:-/tmp}/dosbox-template.XXXXXX.conf")"
trap 'rm -f "$TMP_CONF"' EXIT

cat > "$TMP_CONF" <<EOF
[sdl]
autolock=true

[dosbox]
memsize=64

[cpu]
cycles=max

[autoexec]
mount c "$SCRIPT_DIR"
c:
cd \bin
$APP_NAME
echo Press Ctrl-F9 to exit DOSBox
EOF

printf 'Launching DOSBox with %s\n' "$APP_PATH"
"$DOSBOX_BIN" -conf "$TMP_CONF"
