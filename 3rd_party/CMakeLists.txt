# =============================================================================

# GIT submodule initialization & update target
add_custom_target(
    submodules
    COMMAND git submodule update --init --recursive --force
    VERBATIM
)

# =============================================================================

# spdlog
file(GLOB_RECURSE SPDLOG_SRCS
    spdlog/src/*.cpp
)

add_library(spdlog STATIC ${SPDLOG_SRCS})
add_dependencies(spdlog submodules)
target_compile_definitions(spdlog PRIVATE SPDLOG_COMPILED_LIB)

# =============================================================================

# strutils
set(TESTS OFF CACHE INTERNAL "Don't build tests")
add_subdirectory(strutils)
add_dependencies(strutils submodules)

