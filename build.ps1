# build.ps1 - Configure and build the project using MinGW + CMake
# Usage:
#   .\build.ps1                  # Debug (default)
#   .\build.ps1 -Config Release  # Release

param(
    [ValidateSet("Debug","Release","RelWithDebInfo","MinSizeRel")]
    [string]$Config = "Debug"
)

$mingwBin = "C:\Users\danil\scoop\apps\mingw\current\bin"
$sdlPrefix = "C:\SDKs\SDL3-3.4.4-MINGW\x86_64-w64-mingw32"
$sdlImagePrefix = "C:\SDKs\SDL3_image-3.4.2-MINGW\x86_64-w64-mingw32"
$buildDir = "$PSScriptRoot\build"

# Add MinGW to PATH for this session
$env:PATH = "$mingwBin;$env:PATH"

Write-Host "Build configuration: $Config" -ForegroundColor Cyan

# Configure
cmake -S "$PSScriptRoot" -B "$buildDir" `
    -G "MinGW Makefiles" `
    -DCMAKE_BUILD_TYPE="$Config" `
    -DCMAKE_C_COMPILER="$mingwBin\gcc.exe" `
    -DCMAKE_CXX_COMPILER="$mingwBin\g++.exe" `
    -DCMAKE_PREFIX_PATH="$sdlPrefix;$sdlImagePrefix"

if ($LASTEXITCODE -ne 0) { Write-Error "CMake configure failed"; exit 1 }

# Build
cmake --build "$buildDir" -j

if ($LASTEXITCODE -ne 0) { Write-Error "Build failed"; exit 1 }

Write-Host "`nBuild succeeded. Run: .\build\$Config\demo-game.exe" -ForegroundColor Green
