# Cable Design Tool – TUI Edition

Terminal UI built with [FTXUI](https://github.com/ArthurSonzogni/FTXUI).  
Data stored in a local **SQLite** database (auto-created on first run).  
**Zero runtime dependencies** – single fully static binary per platform.

Both FTXUI and SQLite are downloaded and compiled automatically by CMake.

---

## Prerequisites

| Tool | macOS | Windows |
|------|-------|---------|
| CMake >= 3.16 | `brew install cmake` | [cmake.org](https://cmake.org/download/) |
| C++17 compiler | `xcode-select --install` | MSVC 2019+ or MSYS2 MinGW |
| Git | `brew install git` | [git-scm.com](https://git-scm.com/) |
| Internet (first build only) | — | — |

---

## Build on macOS (native)

```bash
cmake -B build-mac -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac -j$(sysctl -n hw.logicalcpu)
./build-mac/CableDesign
```

---

## Build on Windows (MSVC)

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\CableDesign.exe
```

## Build on Windows (MSYS2 MinGW)

```bash
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/CableDesign.exe
```

---

## Cross-compile Windows .exe from macOS

### One-time setup

```bash
brew install mingw-w64
```

### Build

```bash
cmake -B build-win \
      -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw.cmake \
      -DCMAKE_BUILD_TYPE=Release
cmake --build build-win -j$(sysctl -n hw.logicalcpu)
```

Output: `build-win/CableDesign.exe`  
Drop onto any 64-bit Windows machine and run – no installer, no DLLs, no .NET.

---

## Database

On first run the app creates `cable_design.db` (SQLite) in the **working directory**  
and seeds it with all 14 conductor sizes from the JSON source data.

Subsequent runs read from the existing database, so you can edit the data  
externally with any SQLite tool (e.g. [DB Browser for SQLite](https://sqlitebrowser.org/)).

If the database cannot be opened, the app falls back to the built-in static  
data and shows a warning in the status bar.

---

## Controls

| Key | Action |
|-----|--------|
| `Tab` / `Shift+Tab` | Cycle between System and Cable Data tabs |
| `Up` / `Down` | Move between fields / menu items |
| `Enter` / `F5` | Run calculation |
| `q` / `Esc` | Quit |

---

## Project structure

```
CableDesignTUI/
├── main.cpp                # UI (FTXUI)
├── DatabaseManager.h/.cpp  # SQLite wrapper (no Qt, no system SQLite needed)
├── CableData.h             # Static seed data + CableRecord struct (header-only)
├── Calculator.h            # Calculation engine (header-only)
├── CMakeLists.txt          # Fetches FTXUI + SQLite amalgamation automatically
├── toolchain-mingw.cmake   # Cross-compile Windows .exe from macOS
└── README.md
```
