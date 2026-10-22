cmake_minimum_required(VERSION 3.10)

# Set the project name
project(SdfTask CXX)

# Find the SDL2 library
find_package(SDL2 REQUIRED)

# Uncomment the following line to enable OpenMP
# find_package(OpenMP REQUIRED)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# Include SDL2 headers
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${SDL2_INCLUDE_DIRS})

add_compile_definitions(USE_STB_IMAGE)

# Add the executable
add_executable(render
    main.cpp
    utils/mesh.cpp)

# Link the SDL2 library to the executable
target_link_libraries(render ${SDL2_LIBRARIES})

# Set path to executable
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR})
