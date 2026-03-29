#!/bin/bash -e

# NOTE: this code is mostly copied from luanti_android_deps
# <https://github.com/luanti-org/luanti_android_deps>

glew_ver=2.2.0

ndk_version=r29
ndk_version_n=29.0.14206865

# Build tools and stuff
sudo apt-get update
sudo apt-get install -y wget unzip zip gcc-multilib make cmake

# NDK
if [ -d "/usr/local/lib/android/sdk/ndk/${ndk_version_n}" ]; then
	echo "Found system-wide NDK"
	ndkpath="/usr/local/lib/android/sdk/ndk/${ndk_version_n}"
else
	wget --progress=bar:force "http://dl.google.com/android/repository/android-ndk-${ndk_version}-linux.zip"
	unzip -q "android-ndk-${ndk_version}-linux.zip"
	rm "android-ndk-${ndk_version}-linux.zip"
	ndkpath="$PWD/android-ndk-${ndk_version}"
fi
if ! grep -qF "${ndk_version_n}" "${ndkpath}/source.properties"; then
	echo "NDK version mismatch"
	exit 1
fi

printf 'export ANDROID_NDK="%s"\n' "${ndkpath}"

get_tar_archive () {
	# $1: folder to extract to, $2: URL
	local filename="${2##*/}"
	[ -d "$1" ] && return 0
	wget -c "$2" -O "$filename"
	mkdir -p "$1"
	tar -xaf "$filename" -C "$1" --strip-components=1
	rm "$filename"
}

make_install_copy () {
	make DESTDIR=$PWD install
	mv usr/local/lib/*.a $pkgdir/
	if [ -d $pkgdir/include ]; then
		cp -a usr/local/include $pkgdir/
	else
		mv usr/local/include $pkgdir/
	fi
}

build () {
	make clean glew.lib.static
	make_install_copy
}

_setup_toolchain () {
	local toolchain=$(echo "$ANDROID_NDK"/toolchains/llvm/prebuilt/*)
	if [ ! -d "$toolchain" ]; then
		echo "Android NDK path not specified or incorrect"; return 1
	fi
	export PATH="$toolchain/bin:$ANDROID_NDK:$PATH"

	unset CFLAGS CPPFLAGS CXXFLAGS

	TARGET_ABI="$1"
	API=21
	if [ "$TARGET_ABI" == armeabi-v7a ]; then
		CROSS_PREFIX=armv7a-linux-androideabi
		CFLAGS="-mthumb"
		CXXFLAGS="-mthumb"
	elif [ "$TARGET_ABI" == arm64-v8a ]; then
		CROSS_PREFIX=aarch64-linux-android
	elif [ "$TARGET_ABI" == x86 ]; then
		CROSS_PREFIX=i686-linux-android
		CFLAGS="-mssse3 -mfpmath=sse"
		CXXFLAGS="-mssse3 -mfpmath=sse"
	elif [ "$TARGET_ABI" == x86_64 ]; then
		CROSS_PREFIX=x86_64-linux-android
	else
		echo "Invalid ABI given"; return 1
	fi
	export CC=$CROSS_PREFIX$API-clang
	export CXX=$CROSS_PREFIX$API-clang++
	export AR=llvm-ar
	export RANLIB=llvm-ranlib
	export CFLAGS="-fPIC ${CFLAGS}"
	export CXXFLAGS="-fPIC ${CXXFLAGS}"

	CMAKE_FLAGS=(
		"-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake"
		"-DANDROID_ABI=$TARGET_ABI" "-DANDROID_NATIVE_API_LEVEL=$API"
		"-DCMAKE_BUILD_TYPE=Release"
	)

	export MAKEFLAGS="-j$(nproc)"
}

_run_build () {
	local abi=$1

	mkdir -p src
	pushd src
	srcdir=$PWD
	get_tar_archive glew "https://github.com/nigels-com/glew/releases/download/glew-${glew_ver}/glew-${glew_ver}.tgz"
	popd

	builddir=$srcdir/glew
	pkgdir=$PWD/deps/$abi/glew
	rm -rf "$pkgdir"
	mkdir -p "$pkgdir"

	pushd "$builddir"
	build
	popd

	echo "Build of glew for ABI $abi successful."
}

for abi in armeabi-v7a arm64-v8a x86 x86_64; do
	_setup_toolchain $abi
	_run_build $abi
done

exit 0
