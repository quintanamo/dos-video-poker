#!/bin/bash
set -u

SRC_DIR="${SRC_DIR:-./src}"
BIN_DIR="${BIN_DIR:-./bin}"
OBJ_DIR="${OBJ_DIR:-./obj}"
ERR_DIR="${ERR_DIR:-./err}"
OUTPUT_EXE="${OUTPUT_EXE:-program.exe}"

mkdir -p "$BIN_DIR" "$OBJ_DIR" "$ERR_DIR"
rm -rf "$BIN_DIR"/* "$OBJ_DIR"/* "$ERR_DIR"/* 2>/dev/null || true

echo "Cleaning previous build files..."
echo "Source directory: $SRC_DIR"
echo "Object directory: $OBJ_DIR"
echo "Binary output: $BIN_DIR/$OUTPUT_EXE"
echo "Error directory: $ERR_DIR"

if ! command -v wcl >/dev/null 2>&1; then
    echo "Error: Open Watcom 'wcl' was not found in PATH."
    echo "Install Open Watcom or use the Docker helper script to build in a container."
    exit 1
fi

shopt -s nullglob
SRC_FILES=("$SRC_DIR"/*.c "$SRC_DIR"/*.cpp)
if [ "${#SRC_FILES[@]}" -eq 0 ]; then
    echo "No source files found in $SRC_DIR"
    exit 1
fi

echo "Source files found:"
printf '  %s\n' "${SRC_FILES[@]}"

export WATCOM_TARGET="${WATCOM_TARGET:-DOS}"

WCL_FLAGS=(-bcl=dos -zq -k32768)
if [ "${REAL_HARDWARE:-0}" = "1" ]; then
    WCL_FLAGS+=(-dREAL_HARDWARE=1)
fi

if [ -n "${INCLUDE:-}" ]; then
    WCL_FLAGS+=(-I"$INCLUDE")
fi

wcl "${WCL_FLAGS[@]}" -fe="$BIN_DIR/$OUTPUT_EXE" -fo="$OBJ_DIR/" "${SRC_FILES[@]}" 2> "$ERR_DIR/build_errors.log"
COMPILE_STATUS=$?

find . -type f -name "*.err" -exec mv {} "$ERR_DIR" \; 2>/dev/null || true

if [ "$COMPILE_STATUS" -eq 0 ] && [ -f "$BIN_DIR/$OUTPUT_EXE" ]; then
    echo "Compilation successful. Executable: $BIN_DIR/$OUTPUT_EXE"
    [ ! -s "$ERR_DIR/build_errors.log" ] && rm -f "$ERR_DIR/build_errors.log"
    exit 0
fi

echo "Compilation failed. Check $ERR_DIR/build_errors.log"
exit 1