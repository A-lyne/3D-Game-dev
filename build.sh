#!/bin/bash

# 3D Game Development Build Script for macOS M1 Pro

set -e  # Exit on any error

echo "🎮 3D Game Development Build Script"
echo "===================================="

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "❌ Error: Please run this script from the project root directory"
    exit 1
fi

# Create build directory
echo "📁 Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
echo "⚙️  Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja ..

# Build the project
echo "🔨 Building project..."
ninja

echo ""
echo "✅ Build completed successfully!"
echo ""
echo "🚀 Available executables:"
echo "  • Assignment 3: Simple 3D Game"
echo "    cd build/Assignment_3"
echo "    ./Assignment_3"
echo ""
echo "  • Assignment 4: Animated Mixamo Character"
echo "    cd build/Assignment_4"
echo "    ./Assignment_4"
echo ""
echo "🎮 Controls (Assignment 3):"
echo "  • WASD: Move player"
echo "  • Mouse: Look around"
echo "  • ESC: Exit application"
echo ""
echo "🎮 Controls (Assignment 4):"
echo "  • WASD: Move camera"
echo "  • Mouse: Look around"
echo "  • 1: Play Idle animation"
echo "  • 2: Play Snake Hip Hop Dance animation"
echo "  • ESC: Exit application"
echo ""
echo "🎯 Press ESC in the window to exit"