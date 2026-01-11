#!/bin/bash

set -e

echo "Starting installation..."

# -------------------------------
# Install raylib (only if not present)
# -------------------------------
if [ ! -d "raylib" ]; then
    echo "raylib not found. Installing raylib..."

    git clone --depth=1 https://github.com/raysan5/raylib
    cd raylib
    mkdir build
    cd build
    cmake -DBUILD_SHARED_LIBS=ON ..
    make -j$(nproc)
    sudo make install

    cd ../..
else
    echo "raylib already exists. Skipping."
fi

# -------------------------------
# Install required system libraries
# -------------------------------
echo "Installing system dependencies..."
sudo apt update
sudo apt install -y \
    libcurl4-openssl-dev \
    libcjson-dev \
    pkg-config

echo "Installation completed successfully!"

