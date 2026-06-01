set -e

BUILD_DIR="build"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ ! -f "CMakeCache.txt" ]; then
    echo "Running CMake configuration.."

    if [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32"* ]]; then
        cmake -G "MinGW Makefiles" ..
    else
        cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..
    fi
else
    echo "build directory already configured."
fi

cmake --build .
cd ..

if [ -d "./bin/FrameMeter" ] && [ ! -z "$GGXXACPR_MOD_DIR" ]; then
    echo # line break
    echo "Copying files to mod folder:"
    cp -ru --verbose "./bin/FrameMeter" "$GGXXACPR_MOD_DIR"
fi
