################################################################################
# Set minimum required version of cmake, project name and compile options
################################################################################
cmake_minimum_required(VERSION 3.20)
project(q-space_invaders)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-g -Wall -Wextra -Wpedantic)
endif()


################################################################################
# Find packages and libraries dependencies
################################################################################
find_package(ale REQUIRED)
find_package(OpenCV REQUIRED)


################################################################################
# Build
################################################################################
include_directories(src ${OpenCV_INCLUDE_DIRS})

# executable binaries - Q-Learning Space Invaders
add_executable(qsi src/main.cpp)

# link executable binaries to shared object libraries
target_link_libraries(qsi ale::ale-lib ${OpenCV_LIBS})
