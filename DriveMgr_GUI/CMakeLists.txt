cmake_minimum_required(VERSION 3.10)
project(DriveMgr LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Enable PIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

# Add GTK4 flags
add_compile_options(${GTK4_CFLAGS_OTHER})
include_directories(${GTK4_INCLUDE_DIRS})
link_directories(${GTK4_LIBRARY_DIRS})

# Include our headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add the C++ library
add_library(drivemgr_cpp STATIC
    src/functions.cpp
)

# Link C++ library with OpenSSL
target_link_libraries(drivemgr_cpp
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    stdc++fs  # For std::filesystem
)

# Add the main executable
add_executable(drivemgr src/main.c)

# Link everything together
target_link_libraries(drivemgr
    PRIVATE
    drivemgr_cpp
    ${GTK4_LIBRARIES}
)
