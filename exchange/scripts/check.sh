
#!/usr/bin/env bash

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo "==> Checking formatting..."

FILES=$(find . \
    \( -name '*.hpp' -o -name '*.cpp' \) \
    -not -path './build/*' \
    -not -path './build-ninja/*' \
    -not -path './build-ninja-mingw/*' \
    -not -path './out/*' \
    -not -path './.git/*')

clang-format --dry-run --Werror $FILES

if [[ "$(uname -s)" == MINGW* || "$(uname -s)" == MSYS* ]]; then

    echo "==> Windows detected. Locating Visual Studio 2022..."

    VSWHERE="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"

    if [[ ! -f "$VSWHERE" ]]; then
        echo "ERROR: vswhere.exe not found:"
        echo "       $VSWHERE"
        exit 1
    fi

    VSINSTALL_PATH="$("$VSWHERE" \
        -latest \
        -products '*' \
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 \
        -property installationPath)"

    if [[ -z "$VSINSTALL_PATH" ]]; then
        echo "ERROR: Visual Studio C++ x64 tools were not found."
        exit 1
    fi

    echo "    Visual Studio: $VSINSTALL_PATH"

    VSDEVCMD="$VSINSTALL_PATH/Common7/Tools/VsDevCmd.bat"

    if [[ ! -f "$VSDEVCMD" ]]; then
        echo "ERROR: VsDevCmd.bat not found:"
        echo "       $VSDEVCMD"
        exit 1
    fi

    echo "==> Preparing Visual Studio x64 build..."

    PROJECT_ROOT_WIN="$(cygpath -w "$PROJECT_ROOT")"
    VSDEVCMD_WIN="$(cygpath -w "$VSDEVCMD")"

    BAT_FILE="$(mktemp --suffix=.bat)"

    cat > "$BAT_FILE" <<EOF
@echo off
call "$VSDEVCMD_WIN" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b 1

cd /d "$PROJECT_ROOT_WIN"
if errorlevel 1 exit /b 1

echo.
echo ==^> Compiler:
where cl
if errorlevel 1 exit /b 1

echo.
echo ==^> CMake:
where cmake

echo.
echo ==^> Ninja:
where ninja

echo.
echo ==^> Configuring...
cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 exit /b 1

echo.
echo ==^> Building...
cmake --build build-ninja --parallel
if errorlevel 1 exit /b 1

echo.
echo ==^> Running tests...
ctest --test-dir build-ninja --output-on-failure
if errorlevel 1 exit /b 1
EOF

    BAT_FILE_WIN="$(cygpath -w "$BAT_FILE")"

    cmd.exe //c "$BAT_FILE_WIN"
    RESULT=$?

    rm -f "$BAT_FILE"

    if [[ $RESULT -ne 0 ]]; then
        echo "ERROR: Visual Studio build/test phase failed."
        exit "$RESULT"
    fi

else

    echo "==> Configuring..."

    cmake -S . -B build-ninja -G Ninja \
        -DCMAKE_BUILD_TYPE=Debug

    echo "==> Building..."

    cmake --build build-ninja --parallel

    echo "==> Running tests..."

    ctest --test-dir build-ninja --output-on-failure

fi

echo "==> Running clang-tidy..."

TIDY_FILES=$(find \
    common \
    gateway \
    risk \
    matching_engine \
    market_data \
    persistence \
    client \
    tests \
    -name '*.cpp')

run-clang-tidy -p build-ninja $TIDY_FILES

echo "All checks passed."
```
