#!/usr/bin/env bash

set -euxo pipefail

# Build the Docker image
docker build ./toolchain -t "himbeer/kindle-touch-doom"

# Run the Docker image
docker run --rm -v ./doomgeneric:/source himbeer/kindle-touch-doom -c "cd /source && make clean && make"
