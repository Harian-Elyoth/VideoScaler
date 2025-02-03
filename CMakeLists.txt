cmake_minimum_required(VERSION 3.10)
project(VideoScaler)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find OpenMP
find_package(OpenMP REQUIRED)

# Set optimization flags for different build types
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")  # Enable CPU-specific optimizations
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -mfma")   # Enable AVX2 and FMA
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")             # Maximum optimization for release
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")                     # No optimization for debug

# Enable Link Time Optimization
include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)
if(supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Set the build type to Release by default for better performance
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Add source files
set(SOURCES
    src/main.cpp
    src/VideoProcessor.cpp
    src/FrameScaler.cpp
    src/VideoReader.cpp
    src/VideoWriter.cpp
)

# Set the output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

# Include directories
include_directories(include)

# Add executable
add_executable(VideoScaler ${SOURCES})

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(SWSCALE REQUIRED libswscale)

include_directories(${AVFORMAT_INCLUDE_DIRS} ${AVCODEC_INCLUDE_DIRS} ${AVUTIL_INCLUDE_DIRS} ${SWSCALE_INCLUDE_DIRS})
link_directories(${AVFORMAT_LIBRARY_DIRS} ${AVCODEC_LIBRARY_DIRS} ${AVUTIL_LIBRARY_DIRS} ${SWSCALE_LIBRARY_DIRS})

# Link libraries and add OpenMP
target_link_libraries(VideoScaler 
    ${AVFORMAT_LIBRARIES} 
    ${AVCODEC_LIBRARIES} 
    ${AVUTIL_LIBRARIES} 
    ${SWSCALE_LIBRARIES}
    OpenMP::OpenMP_CXX
)
