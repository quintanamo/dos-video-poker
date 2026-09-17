#!/bin/bash
set -euo pipefail

IMAGE_NAME="${IMAGE_NAME:-ow-dos}"
DOCKERFILE_PATH="${DOCKERFILE_PATH:-Dockerfile}"
PROJECT_DIR="${PROJECT_DIR:-$(pwd)}"
BUILD_SCRIPT="${BUILD_SCRIPT:-build.sh}"

if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
    echo ">> Building Docker image '$IMAGE_NAME'..."
    docker build -t "$IMAGE_NAME" -f "$DOCKERFILE_PATH" "$PROJECT_DIR"
else
    echo ">> Docker image '$IMAGE_NAME' already exists."
fi

if [ ! -f "$PROJECT_DIR/$BUILD_SCRIPT" ]; then
    echo "ERROR: $BUILD_SCRIPT not found in $PROJECT_DIR"
    exit 1
fi

chmod +x "$PROJECT_DIR/$BUILD_SCRIPT"

echo ">> Running build script inside Docker..."
docker run --rm \
    -v "$PROJECT_DIR":/src \
    -w /src \
    "$IMAGE_NAME" \
    "./$BUILD_SCRIPT" "$@"

echo ">> Build complete."
