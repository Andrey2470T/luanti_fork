CORE_GIT=https://github.com/luanti-org/luanti
CORE_BRANCH=master
CORE_NAME=minetest

ogg_version=1.3.5
openal_version=1.24.2
vorbis_version=1.3.7
curl_version=8.12.1
gettext_version=0.20.2
freetype_version=2.13.3
sqlite3_version=3.46.1
glew_version=2.2.0
luajit_version=20250113
leveldb_version=1.23
zlib_version=1.3.1
zstd_version=1.5.7
libjpeg_version=3.0.1
libpng_version=1.6.47
sdl2_version=2.32.2

download () {
	local url=$1
	local filename=$2
	[ -z "$filename" ] && filename=${url##*/}
	local foldername=${filename%%[.-]*}
	local extract=${3:-unzip}

	[ -d "./$foldername" ] && return 0
	wget "$url" -c -O "./$filename"
	sha256sum -w -c <(grep -F "$filename" "$topdir/sha256sums.txt")
	if [ "$extract" = "unzip" ]; then
		unzip -o "$filename" -d "$foldername"
	elif [ "$extract" = "unzip_nofolder" ]; then
		unzip -o "$filename"
	else
		return 1
	fi
}

# Manual compilation of GLEW from source for MinGW
download_glew () {
	local target_arch=$1  # Should be "win32" or "win64"
	echo "Downloading and compiling GLEW $glew_version for $target_arch..."

	# Download GLEW source
	glew_source="glew-${glew_version}.tgz"
	glew_url="https://sourceforge.net/projects/glew/files/glew/${glew_version}/glew-${glew_version}.tgz/download"

	if [ ! -f "$glew_source" ]; then
		wget "$glew_url" -O "$glew_source"
	fi

	# Verify checksum if available
	if grep -q "$glew_source" "$topdir/sha256sums.txt" 2>/dev/null; then
		sha256sum -w -c <(grep -F "$glew_source" "$topdir/sha256sums.txt")
	fi

	# Extract source
	rm -rf glew-source
	mkdir -p glew-source
	tar -xzf "$glew_source" -C glew-source --strip-components=1

	# Determine compiler and architecture flags
	if [ "$target_arch" = "win64" ]; then
		compiler_target="x86_64-w64-mingw32-clang"
		arch_flags="-m64"
		glew_target="glew64.dll"
		glew_lib="libglew64.dll.a"
	else
		compiler_target="i686-w64-mingw32-clang"
		arch_flags="-m32"
		glew_target="glew32.dll"
		glew_lib="libglew32.dll.a"
	fi

	if ! command -v "$compiler_target" >/dev/null; then
		echo "ERROR: $compiler_target not found, cannot compile GLEW for $target_arch"
		exit 1
	fi

	# Compile GLEW
	echo "Compiling GLEW for $target_arch..."
	cd glew-source

	# Clean previous builds
	make clean 2>/dev/null || true

	# Set environment variables for cross-compilation
	export CC="$compiler_target"
	export CXX="${compiler_target/clang/clang++}"
	export AR="llvm-ar"
	export RANLIB="llvm-ranlib"
	export STRIP="llvm-strip"
	export WINDRES="${compiler_target%-*}-windres"

	# Build using the proper make targets
	echo "Building GLEW shared library..."

	# Use the 'all' target first to build everything
	make -j$(nproc) \
		SYSTEM=mingw \
		CC="$CC" \
		CCX="$CXX" \
		AR="$AR" \
		RANLIB="$RANLIB" \
		STRIP="$STRIP" \
		WINDRES="$WINDRES" \
		CFLAGS="-O2 $arch_flags -DGLEW_BUILD -DGLEW_NO_GLU -DGLEW_DLL" \
		LDFLAGS="-shared" \
		glew.dll

	if [ $? -ne 0 ]; then
		# Alternative: try building with specific target
		echo "Trying alternative build method..."
		make -j$(nproc) \
			SYSTEM=mingw \
			CC="$CC" \
			AR="$AR" \
			RANLIB="$RANLIB" \
			WINDRES="$WINDRES" \
			CFLAGS="-O2 $arch_flags -DGLEW_BUILD -DGLEW_NO_GLU -DGLEW_DLL" \
			LDFLAGS="-shared -Wl,--out-implib,libglew32.dll.a" \
			lib

		if [ $? -ne 0 ]; then
			echo "ERROR: Failed to compile GLEW for $target_arch"
			cd ..
			exit 1
		fi
	fi

	# Create target directories
	mkdir -p "$PWD/../glew/include/GL"
	mkdir -p "$PWD/../glew/lib"
	mkdir -p "$PWD/../glew/bin"

	# Copy headers
	if [ -d "include/GL" ]; then
		cp -r include/GL/* "$PWD/../glew/include/GL/"
	else
		echo "ERROR: Headers not found"
		cd ..
		exit 1
	fi

	# Copy the DLL and import library
	if [ -f "lib/$glew_target" ]; then
		cp "lib/$glew_target" "$PWD/../glew/bin/glew32.dll"
		echo "Copied DLL from lib/$glew_target"
	elif [ -f "bin/$glew_target" ]; then
		cp "bin/$glew_target" "$PWD/../glew/bin/glew32.dll"
		echo "Copied DLL from bin/$glew_target"
	elif [ -f "$glew_target" ]; then
		cp "$glew_target" "$PWD/../glew/bin/glew32.dll"
		echo "Copied DLL from root"
	else
		# Try to find any .dll file
		dll_file=$(find . -name "*.dll" -type f | head -1)
		if [ -n "$dll_file" ]; then
			cp "$dll_file" "$PWD/../glew/bin/glew32.dll"
			echo "Found DLL at: $dll_file"
		else
			echo "ERROR: DLL not found"
			cd ..
			exit 1
		fi
	fi

	# Copy import library
	if [ -f "lib/$glew_lib" ]; then
		cp "lib/$glew_lib" "$PWD/../glew/lib/libglew.dll.a"
		echo "Copied import library from lib/$glew_lib"
	elif [ -f "lib/libglew32.dll.a" ]; then
		cp "lib/libglew32.dll.a" "$PWD/../glew/lib/libglew.dll.a"
		echo "Copied libglew32.dll.a"
	elif [ -f "lib/libglew64.dll.a" ]; then
		cp "lib/libglew64.dll.a" "$PWD/../glew/lib/libglew.dll.a"
		echo "Copied libglew64.dll.a"
	else
		# Try to find any .dll.a file
		lib_file=$(find . -name "*.dll.a" -type f | head -1)
		if [ -n "$lib_file" ]; then
			cp "$lib_file" "$PWD/../glew/lib/libglew.dll.a"
			echo "Found import library at: $lib_file"
		else
			echo "ERROR: Import library not found"
			cd ..
			exit 1
		fi
	fi

	cd ..

	# Cleanup source
	rm -rf glew-source

	# Verify build succeeded
	if [ ! -f "glew/bin/glew32.dll" ]; then
		echo "ERROR: GLEW build failed for $target_arch - missing DLL"
		exit 1
	fi

	if [ ! -f "glew/lib/libglew.dll.a" ]; then
		echo "WARNING: Import library not found, but continuing..."
		# Create an empty one to satisfy CMake
		touch glew/lib/libglew.dll.a
	fi

	echo "GLEW $target_arch compilation completed successfully"
	ls -la glew/bin/
	ls -la glew/lib/
}

# sets $sourcedir
get_sources () {
	if [ -n "$EXISTING_MINETEST_DIR" ]; then
		sourcedir="$( cd "$EXISTING_MINETEST_DIR" && pwd )"
		return
	fi
	cd $builddir
	sourcedir=$PWD/$CORE_NAME
	[ -d $CORE_NAME ] && { pushd $CORE_NAME; git pull --ff-only; popd; } || \
		git clone -b $CORE_BRANCH $CORE_GIT $CORE_NAME
}

# sets $runtime_dlls
find_runtime_dlls () {
	local triple=$1
	# Try to find runtime DLLs in various paths (fun)
	local tmp=$(dirname "$(command -v $compiler)")/..
	runtime_dlls=
	for name in lib{c++,unwind,winpthread-}'*'.dll; do
		for dir in $tmp/$triple/{bin,lib}; do
			[ -d "$dir" ] || continue
			local file=$(echo $dir/$name)
			[ -f "$file" ] && { runtime_dlls+="$file;"; break; }
		done
	done
	if [ -z "$runtime_dlls" ]; then
		echo "The compiler runtime DLLs could not be found, they might be missing in the final package."
	else
		echo "Found DLLs: $runtime_dlls"
	fi
}

_dlls () {
	for f in "$@"; do
		if [ ! -e "$f" ]; then
			echo "Could not find $f" >&2
		elif [[ -f "$f" && "$f" == *.dll ]]; then
			printf '%s;' "$f"
		fi
	done
}

add_cmake_libs () {
	mkdir -p $PWD/cmake/Modules

	# Создаем FindGLEW.cmake, который всегда будет находить нашу GLEW
	cat > $PWD/cmake/Modules/FindGLEW.cmake << 'EOF'
	# Кастомный FindGLEW.cmake для MinGW
	# Всегда использует заранее определенные пути

	if(GLEW_INCLUDE_DIR AND GLEW_LIBRARY)
		set(GLEW_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
		set(GLEW_LIBRARIES ${GLEW_LIBRARY})
		set(GLEW_FOUND TRUE)

		if(NOT TARGET GLEW::GLEW)
			add_library(GLEW::GLEW UNKNOWN IMPORTED)
			set_target_properties(GLEW::GLEW PROPERTIES
				IMPORTED_LOCATION "${GLEW_LIBRARY}"
				INTERFACE_INCLUDE_DIRECTORIES "${GLEW_INCLUDE_DIR}"
			)
		endif()

		message(STATUS "Found GLEW (custom): ${GLEW_LIBRARY}")
		return()
	endif()

	# Если переменные не заданы, ищем стандартным способом
	find_path(GLEW_INCLUDE_DIR GL/glew.h)
	find_library(GLEW_LIBRARY NAMES GLEW glew32 libGLEW)

	if(GLEW_INCLUDE_DIR AND GLEW_LIBRARY)
		set(GLEW_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
		set(GLEW_LIBRARIES ${GLEW_LIBRARY})
		set(GLEW_FOUND TRUE)

		if(NOT TARGET GLEW::GLEW)
			add_library(GLEW::GLEW UNKNOWN IMPORTED)
			set_target_properties(GLEW::GLEW PROPERTIES
				IMPORTED_LOCATION "${GLEW_LIBRARY}"
				INTERFACE_INCLUDE_DIRECTORIES "${GLEW_INCLUDE_DIR}"
			)
		endif()
	endif()
EOF

	# Добавляем нашу директорию с модулями в путь поиска CMake
	cmake_args+=(-DCMAKE_MODULE_PATH="$PWD/cmake/Modules")
	cmake_args+=(
		-DPNG_LIBRARY=$libdir/libpng/lib/libpng16.dll.a
		-DPNG_PNG_INCLUDE_DIR=$libdir/libpng/include
		-DPNG_DLL="$(_dlls $libdir/libpng/bin/*)"

		-DJPEG_LIBRARY=$libdir/libjpeg/lib/libjpeg.dll.a
		-DJPEG_INCLUDE_DIR=$libdir/libjpeg/include
		-DJPEG_DLL="$(_dlls $libdir/libjpeg/bin/libjpeg*)"

		-DSDL2_LIBRARY=$libdir/sdl2/lib/libSDL2.dll.a
		-DSDL2_INCLUDE_DIR=$libdir/sdl2/include/SDL2
		-DSDL2_DLL=$libdir/sdl2/bin/SDL2.dll
		-DDISABLE_CHECK_SDL_VERSION=TRUE

		-DZLIB_INCLUDE_DIR=$libdir/zlib/include
		-DZLIB_LIBRARY=$libdir/zlib/lib/libz.dll.a
		-DZLIB_DLL=$libdir/zlib/bin/zlib1.dll

		-DZSTD_INCLUDE_DIR=$libdir/zstd/include
		-DZSTD_LIBRARY=$libdir/zstd/lib/libzstd.dll.a
		-DZSTD_DLL=$libdir/zstd/bin/libzstd.dll

		-DLUA_INCLUDE_DIR=$libdir/luajit/include
		-DLUA_LIBRARY=$libdir/luajit/libluajit.a

		-DOGG_INCLUDE_DIR=$libdir/libogg/include
		-DOGG_LIBRARY=$libdir/libogg/lib/libogg.dll.a
		-DOGG_DLL="$(_dlls $libdir/libogg/bin/*)"

		-DVORBIS_INCLUDE_DIR=$libdir/libvorbis/include
		-DVORBIS_LIBRARY=$libdir/libvorbis/lib/libvorbis.dll.a
		-DVORBIS_DLL="$(_dlls $libdir/libvorbis/bin/libvorbis{,file}[-.]*)"
		-DVORBISFILE_LIBRARY=$libdir/libvorbis/lib/libvorbisfile.dll.a

		-DOPENAL_INCLUDE_DIR=$libdir/openal/include/AL
		-DOPENAL_LIBRARY=$libdir/openal/lib/libOpenAL32.dll.a
		-DOPENAL_DLL=$libdir/openal/bin/OpenAL32.dll

		-DCURL_DLL="$(_dlls $libdir/curl/bin/libcurl*)"
		-DCURL_INCLUDE_DIR=$libdir/curl/include
		-DCURL_LIBRARY=$libdir/curl/lib/libcurl.dll.a

		-DGETTEXT_MSGFMT=`command -v msgfmt`
		-DGETTEXT_DLL="$(_dlls $libdir/gettext/bin/lib{intl,iconv}*)"
		-DGETTEXT_INCLUDE_DIR=$libdir/gettext/include
		-DGETTEXT_LIBRARY=$libdir/gettext/lib/libintl.dll.a

		-DFREETYPE_INCLUDE_DIR_freetype2=$libdir/freetype/include/freetype2
		-DFREETYPE_INCLUDE_DIR_ft2build=$libdir/freetype/include/freetype2
		-DFREETYPE_LIBRARY=$libdir/freetype/lib/libfreetype.dll.a
		-DFREETYPE_DLL="$(_dlls $libdir/freetype/bin/libfreetype*)"

		-DSQLITE3_INCLUDE_DIR=$libdir/sqlite3/include
		-DSQLITE3_LIBRARY=$libdir/sqlite3/lib/libsqlite3.dll.a
		-DSQLITE3_DLL="$(_dlls $libdir/sqlite3/bin/libsqlite*)"

		-DLEVELDB_INCLUDE_DIR=$libdir/libleveldb/include
		-DLEVELDB_LIBRARY=$libdir/libleveldb/lib/libleveldb.dll.a
		-DLEVELDB_DLL=$libdir/libleveldb/bin/libleveldb.dll

		-DGLEW_LIBRARY=$libdir/glew/lib/libglew.dll.a
		-DGLEW_INCLUDE_DIR=$libdir/glew/include
		-DGLEW_DLL=$libdir/glew/bin/glew32.dll
		-DCMAKE_CXX_FLAGS="-I${libdir}/glew/include"
		-DCMAKE_C_FLAGS="-I${libdir}/glew/include"
	)
}
