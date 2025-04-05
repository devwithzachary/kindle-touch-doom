#!/usr/bin/env bash

set -euxo pipefail

SDK_TARGET="kindlepw2"

# Set the SDK target to the first argument if it is provided
if [ "$#" -eq 1 ]; then
    SDK_TARGET="$1"
fi

echo "Building for SDK target: ${SDK_TARGET}"

# Build the Docker image
docker build ./toolchain -t "himbeer/kindle-touch-doom-${SDK_TARGET}" --build-arg "SDK_TARGET=${SDK_TARGET}"

# Run the Docker image
docker run --rm -v ./doomgeneric:/source "himbeer/kindle-touch-doom-${SDK_TARGET}" -c "cd /source && make clean && make"
