cmake_minimum_required(VERSION 3.2)
project(Synth VERSION 0.1.0)

# Options
option(USE_PORTAUDIO "Enable PortAudio backend" ON)
option(USE_TBB "Use the Thread Building Blocks (TBB) library" ON)
option(TRACE "Enable spdlog trace level" OFF)

# Set the default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# Initialize GIT submodules
execute_process(COMMAND git submodule update --init --recursive --force)

# =============================================================================

# Detect system architecture
execute_process(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCH)
message(STATUS "Architecture: ${ARCH}")

message(STATUS "Build type  : ${CMAKE_BUILD_TYPE}")
message(STATUS "Trace       : ${TRACE}")

# =============================================================================

# Setup common compilation flags
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Wextra -Wno-unused-function")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -ffast-math -O3")

# Platform specific flags
if(${ARCH} STREQUAL "armv7l")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard")
endif()

# Add symbols if requested
if(DEBUG_SYMBOLS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")   
endif()

# Common include and link directories
include_directories(src)
include_directories(3rd_party)
include_directories(3rd_party/spdlog/include)
include_directories(3rd_party/strutils/include)

# Common defines
add_definitions(-DARCH="${ARCH}")
add_definitions(-DSPDLOG_COMPILED_LIB)

if(USE_TBB)
    add_definitions(-DSYNTH_USE_TBB)
endif()

if(USE_PORTAUDIO)
    add_definitions(-DSYNTH_USE_PORTAUDIO)
endif()

# Enable trace
if(TRACE)
    add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
else()
    add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

# libxml
find_package(LibXml2)
if(NOT LIBXML2_FOUND)
    message(FATAL_ERROR "libxml not found!")
endif()

include_directories(${LIBXML2_INCLUDE_DIRS})

# libtbb
if(USE_TBB)
    find_package(TBB)
    if(NOT TBB_FOUND)
        message(FATAL_ERROR "libtbb not found!")
    endif()

    include_directories(${TBB_INCLUDE_DIRS})
endif()

# =============================================================================

# Include 3rd party library targets
add_subdirectory(3rd_party)

# 3rd party libs
set(THIRD_PARTY_LIBS
    spdlog
    strutils
)

# Common libs
set(COMMON_LIBS
    pthread
    ${LIBXML2_LIBRARIES}
)

if(USE_TBB)
    set(COMMON_LIBS ${COMMON_LIBS} tbb)
endif()

# Audio libs
set(AUDIO_LIBS
    sndfile
    asound
)

if(USE_PORTAUDIO)
    set(AUDIO_LIBS ${AUDIO_LIBS} portaudio)
endif()

# =============================================================================

# Common sources
file (GLOB_RECURSE SRCS
    src/midi/*.c  src/midi/*.cc
    src/utils/*.c src/utils/*.cc
    src/graph/*.c src/graph/*.cc
    src/iface/*.c src/iface/*.cc
    src/instrument/*.c src/instrument/*.cc    
)

# Audio sources
set (AUDIO_SRCS
    src/audio/audio_sink.cc
    src/audio/alsa_sink.cc
    src/audio/recorder.cc
)

if(USE_PORTAUDIO)
    set(AUDIO_SRCS ${AUDIO_SRCS} src/audio/portaudio_sink.cc)
endif()

# =============================================================================
# The main synth app

set (SYNTH_SRCS
    src/main.cc
    src/app/synth_app.cc
    src/app/synth_commands.cc
)

add_executable(synth ${SRCS} ${AUDIO_SRCS} ${SYNTH_SRCS})

target_link_libraries(synth PRIVATE
    ${COMMON_LIBS}
    ${AUDIO_LIBS}
    ${THIRD_PARTY_LIBS}
)

# =============================================================================
# Benchmarking app

set (BENCHMARK_SRCS
    src/benchmark.cc
    src/app/benchmark_app.cc
)

add_executable(benchmark ${SRCS} ${AUDIO_SRCS} ${BENCHMARK_SRCS})

target_link_libraries(benchmark PRIVATE
    ${COMMON_LIBS}
    ${AUDIO_LIBS}
    ${THIRD_PARTY_LIBS}
)
