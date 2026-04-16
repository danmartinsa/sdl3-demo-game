# Setup Guide — demo-game (SDL3 / MinGW / CMake on Windows)

This document records every step taken to install, configure, and scaffold this project so anyone can reproduce it from scratch on a fresh Windows machine.

---

## Prerequisites

- Windows 10 or 11 (64-bit)
- [Scoop](https://scoop.sh/) package manager

### Install Scoop (if not already installed)

Open PowerShell as a normal user (no admin needed) and run:

```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
Invoke-RestMethod -Uri https://get.scoop.sh | Invoke-Expression
```

---

## 1. Install toolchain via Scoop

```powershell
scoop install git cmake mingw
```

Versions confirmed working:

| Tool  | Version              |
|-------|----------------------|
| git   | 2.53.0.2             |
| cmake | 4.3.1                |
| mingw | 15.2.0-rt_v13-rev1   |

Scoop places the MinGW binaries at:

```
C:\Users\<you>\scoop\apps\mingw\current\bin\
```

> **Note:** Scoop does *not* permanently add MinGW to your system `PATH`. The build script (`build.ps1`) prepends it for the current session automatically.

---

## 2. Download and install SDL3

SDL3 is **not** available via Scoop yet. Download the pre-built MinGW package from the official SDL releases:

```
https://github.com/libsdl-org/SDL/releases
```

Pick the `SDL3-<version>-mingw.zip` (or `.tar.gz`) asset.  
The version used in this project: **SDL3-3.4.4-MINGW**

Extract the archive and place it at:

```
C:\SDKs\SDL3-3.4.4-MINGW\
```

The directory layout should look like:

```
C:\SDKs\SDL3-3.4.4-MINGW\
├── x86_64-w64-mingw32\
│   ├── bin\          ← SDL3.dll lives here
│   ├── include\
│   ├── lib\
│   │   └── cmake\SDL3\   ← CMake config files
│   └── share\
└── i686-w64-mingw32\     ← 32-bit (not used)
```

The CMake prefix path used by this project is:

```
C:\SDKs\SDL3-3.4.4-MINGW\x86_64-w64-mingw32
```

---

## 3. Project structure

After scaffolding, the repository looks like this:

```
demo-game\
├── src\
│   └── main.cpp       ← SDL3 game loop entry point
├── CMakeLists.txt     ← CMake build definition
├── build.ps1          ← One-shot configure + build script
├── SETUP.md           ← This file
└── README.md
```

---

## 4. Build the project

From the project root in PowerShell:

```powershell
.\build.ps1
```

What the script does internally:

```powershell
# Temporarily add MinGW to PATH
$env:PATH = "C:\Users\<you>\scoop\apps\mingw\current\bin;$env:PATH"

# Configure
cmake -S . -B build `
    -G "MinGW Makefiles" `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCMAKE_CXX_COMPILER="...\g++.exe" `
    -DCMAKE_PREFIX_PATH="C:\SDKs\SDL3-3.4.4-MINGW\x86_64-w64-mingw32"

# Compile
cmake --build build -j
```

On success the executable and a copy of `SDL3.dll` appear in `build\`:

```
build\
├── demo-game.exe
└── SDL3.dll
```

---

## 5. Run the game

```powershell
.\build\demo-game.exe
```

A resizable window opens with a dark background. Press **Escape** or close the window to quit.

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `cmake: command not found` | Run `scoop install cmake` |
| `g++.exe not found` | Run `scoop install mingw`; the build script sets the path explicitly |
| `SDL3 not found` | Verify `C:\SDKs\SDL3-3.4.4-MINGW\x86_64-w64-mingw32\lib\cmake\SDL3\SDL3Config.cmake` exists |
| `SDL3.dll not found` at runtime | The post-build step copies it; make sure you run from the `build\` directory or run via `build.ps1` |
