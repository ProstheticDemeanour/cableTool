cmake_minimum_required(VERSION 3.16)

# Set the MSVC static runtime BEFORE project() so it applies to every target
# created afterwards — including those pulled in by FetchContent (FTXUI, etc.).
# This avoids the "cannot set property on ALIAS target" error entirely.
if(MSVC OR (CMAKE_GENERATOR MATCHES "Visual Studio"))
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        CACHE STRING "" FORCE)
endif()

project(cableTool VERSION 0.1.2 LANGUAGES C CXX)

# Opt in to CMP0135 (URL download extract timestamps) to silence the warning
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ── 1. FTXUI ──────────────────────────────────────────────────────────────────
FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
    GIT_TAG        v5.0.0
    GIT_SHALLOW    TRUE
)
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ftxui)

# ── 2. SQLite amalgamation ────────────────────────────────────────────────────
# Downloads sqlite3.c + sqlite3.h and compiles them as a static C library.
# No system SQLite required — works on macOS, Windows (MSVC/MinGW), and
# cross-compilation via toolchain-mingw.cmake.
FetchContent_Declare(sqlite3
    URL                        https://www.sqlite.org/2024/sqlite-amalgamation-3460000.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    # To pin the download add: URL_HASH SHA256=<hash>
)
FetchContent_MakeAvailable(sqlite3)

add_library(sqlite3_lib STATIC
    "${sqlite3_SOURCE_DIR}/sqlite3.c"
)
target_include_directories(sqlite3_lib PUBLIC
    "${sqlite3_SOURCE_DIR}"
)
target_compile_definitions(sqlite3_lib PUBLIC
    SQLITE_THREADSAFE=0           # single-threaded app
    SQLITE_DEFAULT_MEMSTATUS=0    # no memory tracking overhead
    SQLITE_OMIT_LOAD_EXTENSION=1  # no dlopen — simpler linking on Windows
)
# Suppress warnings from third-party code we don't control
if(MSVC)
    target_compile_options(sqlite3_lib PRIVATE /W0)
else()
    target_compile_options(sqlite3_lib PRIVATE -w)
endif()

# ── 3. Main executable ────────────────────────────────────────────────────────
add_executable(cableTool
    src/main.cpp
    db/DatabaseManager.cpp
    # Header-only — no .cpp needed:
    #   db/CableData.h
    #   engine/Calculator.h
)

# Expose all three subdirectories as include paths so headers can be
# included without path prefixes (e.g. #include "CableData.h")
target_include_directories(cableTool PRIVATE
    ${CMAKE_SOURCE_DIR}/db
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(cableTool PRIVATE
    sqlite3_lib
    ftxui::screen
    ftxui::dom
    ftxui::component
)

# ── 4. Platform tweaks ────────────────────────────────────────────────────────
if(MINGW)
    # Statically link libgcc/libstdc++ — fully self-contained .exe
    target_link_options(cableTool PRIVATE -static-libgcc -static-libstdc++)
endif()

if(APPLE)
    set_target_properties(cableTool PROPERTIES
        MACOSX_DEPLOYMENT_TARGET "11.0"
    )
endif()

# ── 5. Build summary ──────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "=== cableTool TUI ===")
message(STATUS "  Host    : ${CMAKE_HOST_SYSTEM_NAME}")
message(STATUS "  Target  : ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Type    : ${CMAKE_BUILD_TYPE}")
message(STATUS "=======================")
message(STATUS "")
