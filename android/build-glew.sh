#!/bin/bash -e

glew_ver=2.2.0

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

for var in ANDROID_NDK ANDROID_NDK_ROOT ANDROID_HOME ANDROID_SDK_ROOT; do
    val="${!var}"
    if [ -n "$val" ] && [ -d "$val" ]; then
        ANDROID_NDK="$val"
        break
    fi
done

if [ -z "$ANDROID_NDK" ]; then
    for dir in /opt/android-sdk/ndk "$ANDROID_HOME/ndk" "$ANDROID_SDK_ROOT/ndk"; do
        if [ -d "$dir" ]; then
            ANDROID_NDK=$(ls -d "$dir"/*/ 2>/dev/null | head -1)
            break
        fi
    done
fi

[ -z "$ANDROID_NDK" ] && echo "Android NDK not found" && exit 1
OUTPUT_DIR="$SCRIPT_DIR/deps"

download_glew() {
    local src_dir="$OUTPUT_DIR/src"
    mkdir -p "$src_dir"

    [ -d "$src_dir/libglew" ] && return 0

    local glew_tar="glew-${glew_ver}.tgz"
    wget -c "https://github.com/nigels-com/glew/releases/download/glew-${glew_ver}/${glew_tar}" -O "$glew_tar"
    mkdir -p "$src_dir/libglew"
    tar -xaf "$glew_tar" -C "$src_dir/libglew" --strip-components=1
    rm "$glew_tar"
}

setup_toolchain() {
    local abi=$1
    local toolchain=$(echo "$ANDROID_NDK"/toolchains/llvm/prebuilt/*)
    [ ! -d "$toolchain" ] && echo "Android NDK not found: $ANDROID_NDK" && return 1

    export PATH="$toolchain/bin:$ANDROID_NDK:$PATH"
    unset CFLAGS CPPFLAGS CXXFLAGS

    API=21
    if [ "$abi" == "armeabi-v7a" ]; then
        CROSS_PREFIX=armv7a-linux-androideabi
        CFLAGS="-mthumb -fPIC"
    elif [ "$abi" == "arm64-v8a" ]; then
        CROSS_PREFIX=aarch64-linux-android
        CFLAGS="-fPIC"
    elif [ "$abi" == "x86" ]; then
        CROSS_PREFIX=i686-linux-android
        CFLAGS="-fPIC -mssse3 -mfpmath=sse"
    elif [ "$abi" == "x86_64" ]; then
        CROSS_PREFIX=x86_64-linux-android
        CFLAGS="-fPIC"
    else
        echo "Invalid ABI: $abi"
        return 1
    fi

    export CC=$CROSS_PREFIX$API-clang
    export CXX=$CROSS_PREFIX$API-clang++
    export AR=llvm-ar
    export RANLIB=llvm-ranlib
    export CFLAGS="$CFLAGS -DGLEW_NO_GLU -DGLEW_STATIC"
    export CXXFLAGS="$CFLAGS"
}

build_glew_abi() {
    local abi=$1
    local src_dir="$OUTPUT_DIR/src/libglew"
    local build_dir="$OUTPUT_DIR/build/$abi"
    local install_dir="$OUTPUT_DIR/$abi"

    echo "Building GLEW for $abi..."

    setup_toolchain $abi || return 1

    mkdir -p "$build_dir" "$install_dir/lib" "$install_dir/include/GL"
    [ ! -f "$build_dir/src/glew.c" ] && cp -r "$src_dir"/* "$build_dir/"

    cd "$build_dir"

    mkdir -p lib
    rm -f lib/*.o lib/*.a

    ${CC} -DGLEW_NO_GLU -DGLEW_STATIC -fPIC -Iinclude -c src/glew.c -o lib/glew.o
    ${AR} rcs lib/libGLEW.a lib/glew.o

    [ -f "lib/libGLEW.a" ] && cp lib/libGLEW.a "$install_dir/lib/" && cp include/GL/glew.h "$install_dir/include/GL/"
}

create_find_glew_cmake() {
    mkdir -p "$OUTPUT_DIR/cmake/Modules"
    cat > "$OUTPUT_DIR/cmake/Modules/FindGLEW.cmake" << 'EOF'
if(GLEW_INCLUDE_DIR AND GLEW_LIBRARY)
    set(GLEW_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
    set(GLEW_LIBRARIES ${GLEW_LIBRARY})
    set(GLEW_FOUND TRUE)
elseif(CUSTOM_GLEW_DIR)
    if(ANDROID_ABI)
        find_path(GLEW_INCLUDE_DIR GL/glew.h
            PATHS "${CUSTOM_GLEW_DIR}/${ANDROID_ABI}/include"
            NO_DEFAULT_PATH)
        find_library(GLEW_LIBRARY NAMES libGLEW GLEW glew32
            PATHS "${CUSTOM_GLEW_DIR}/${ANDROID_ABI}/lib"
            NO_DEFAULT_PATH)
    endif()

    if(NOT GLEW_INCLUDE_DIR OR NOT GLEW_LIBRARY)
        foreach(abi armeabi-v7a arm64-v8a x86 x86_64)
            if(NOT GLEW_INCLUDE_DIR)
                find_path(GLEW_INCLUDE_DIR GL/glew.h
                    PATHS "${CUSTOM_GLEW_DIR}/${abi}/include"
                    NO_DEFAULT_PATH)
            endif()
            if(NOT GLEW_LIBRARY)
                find_library(GLEW_LIBRARY NAMES libGLEW GLEW glew32
                    PATHS "${CUSTOM_GLEW_DIR}/${abi}/lib"
                    NO_DEFAULT_PATH)
            endif()
        endforeach()
    endif()

    if(GLEW_INCLUDE_DIR AND GLEW_LIBRARY)
        set(GLEW_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
        set(GLEW_LIBRARIES ${GLEW_LIBRARY})
        set(GLEW_FOUND TRUE)
    endif()
else()
    find_path(GLEW_INCLUDE_DIR GL/glew.h)
    find_library(GLEW_LIBRARY NAMES GLEW glew32 libGLEW)
    if(GLEW_INCLUDE_DIR AND GLEW_LIBRARY)
        set(GLEW_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
        set(GLEW_LIBRARIES ${GLEW_LIBRARY})
        set(GLEW_FOUND TRUE)
    endif()
endif()

if(GLEW_FOUND)
    if(NOT TARGET GLEW::GLEW)
        add_library(GLEW::GLEW UNKNOWN IMPORTED)
        set_target_properties(GLEW::GLEW PROPERTIES
            IMPORTED_LOCATION "${GLEW_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GLEW_INCLUDE_DIR}")
    endif()
    message(STATUS "Found GLEW: ${GLEW_LIBRARY}")
endif()
EOF
}

build_all() {
    mkdir -p "$OUTPUT_DIR"
    download_glew

    for abi in armeabi-v7a arm64-v8a x86 x86_64; do
        build_glew_abi $abi
    done

    create_find_glew_cmake
    echo "GLEW build completed: $OUTPUT_DIR"
    find "$OUTPUT_DIR" -name "*.a" -type f
}

case "${1:-all}" in
    all) build_all ;;
    clean) rm -rf "$OUTPUT_DIR" ;;
    armeabi-v7a|arm64-v8a|x86|x86_64) download_glew && build_glew_abi $1 ;;
    *) echo "Usage: $0 [all|clean|<abi>]" ;;
esac
