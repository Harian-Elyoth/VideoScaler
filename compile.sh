echo "VideoScaler Project\n"

# Set the build mode based on the first argument, default to "prod"
BUILD_MODE=${1:-prod}

# Set the build type based on the build mode
if [ "$BUILD_MODE" = "debug" ]; then
    BUILD_TYPE="Debug"
else
    BUILD_TYPE="Release"
fi

# Compile the source code
echo "Compiling the source code in $BUILD_TYPE mode..."

# Check if the build repository exists
if [ ! -d "build" ]; then
    mkdir build
fi

# Change to the build directory
pushd build

# Run CMake with the chosen build type
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..

# Build the project
make

# Return to the original directory
popd
