# toolchain-mingw.cmake
# Cross-compile a Windows x86-64 executable from macOS using mingw-w64.
#
# Prerequisites (one-time setup):
#   brew install mingw-w64
#
# Usage:
#   cmake -B build-win \
#         -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw.cmake \
#         -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-win
#
# The resulting build-win/CableDesign.exe runs on any 64-bit Windows system
# with no additional runtimes (FTXUI is static; -static-libgcc/stdc++ is set
# in CMakeLists.txt for MinGW).

set(CMAKE_SYSTEM_NAME    Windows)
set(CMAKE_SYSTEM_VERSION 10)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# ── Locate the mingw-w64 toolchain installed by Homebrew ─────────────────────
# Homebrew on Apple Silicon puts binaries in /opt/homebrew; Intel in /usr/local.
# We search both.
find_program(MINGW_GCC
    NAMES x86_64-w64-mingw32-gcc
    HINTS
        /opt/homebrew/bin           # Apple Silicon
        /usr/local/bin              # Intel Mac
        /opt/homebrew/opt/mingw-w64/bin
        /usr/local/opt/mingw-w64/bin
)

if(NOT MINGW_GCC)
    message(FATAL_ERROR
        "mingw-w64 not found.\n"
        "Install it with:  brew install mingw-w64\n"
        "Then re-run cmake.")
endif()

get_filename_component(MINGW_BIN_DIR "${MINGW_GCC}" DIRECTORY)

set(CMAKE_C_COMPILER   "${MINGW_BIN_DIR}/x86_64-w64-mingw32-gcc")
set(CMAKE_CXX_COMPILER "${MINGW_BIN_DIR}/x86_64-w64-mingw32-g++")
set(CMAKE_RC_COMPILER  "${MINGW_BIN_DIR}/x86_64-w64-mingw32-windres")

# ── Sysroot / find-root ───────────────────────────────────────────────────────
# Tell CMake to look for libraries in the MinGW sysroot, not the macOS sysroot.
set(CMAKE_FIND_ROOT_PATH "${MINGW_BIN_DIR}/..")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)   # host tools (cmake, python…) from host
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)    # libraries from mingw sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)    # headers  from mingw sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ── Windows API version (Vista+, needed for some terminal APIs) ───────────────
add_compile_definitions(
    _WIN32_WINNT=0x0600   # Windows Vista minimum
    WINVER=0x0600
)
