cmake_minimum_required(VERSION 3.10.0)
project(MeshSubdivisionViewer VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add Include Path
include_directories(
	head
	include
)

# find source codes
# Add source files
file(GLOB_RECURSE SOURCE_FILES 
    src/*.cpp
    head/*.h
    src/**/*.cpp
    head/**/*.h
)
if(WIN32)
add_definitions(-D_AMD64_)
endif()
add_executable(MeshSubdivisionViewer WIN32 ${SOURCE_FILES})

if(MSVC)
    target_compile_definitions(MeshSubdivisionViewer PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(MeshSubdivisionViewer PRIVATE /FS)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MeshSubdivisionViewer)
endif()
