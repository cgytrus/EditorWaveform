cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_NAME EditorWaveform)
project(${PROJECT_NAME})

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

# this is so stupid
set(CMAKE_SIZEOF_VOID_P 4)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
  # ensure 32 bit on clang
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target i386-pc-windows-msvc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target i386-pc-windows-msvc")
  add_definitions("--target=i386-pc-windows-msvc")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
  libraries/hackpro/include
  libraries/minhook/include
  libraries/gd.h/include
  libraries/gd.h
  libraries/fmod
)

target_compile_definitions(${PROJECT_NAME} PRIVATE CUSTOM_DEBUG)

add_subdirectory(libraries/minhook)
add_subdirectory(libraries/cocos-headers)
add_subdirectory(libraries/mat-dash)

target_link_libraries(${PROJECT_NAME} minhook cocos2d mat-dash opengl32 ${CMAKE_SOURCE_DIR}/libraries/fmod/fmod.lib)

target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/libraries/hackpro/bin/hackpro.lib)
target_link_options(${PROJECT_NAME} PRIVATE "/DELAYLOAD:hackpro.dll")
