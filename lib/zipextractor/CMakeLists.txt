set(CMAKE_CXX_FLAGS "-std=c++17")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie -lstdc++fs")
endif()

find_package(SDL2 REQUIRED)
find_package(ZLIB REQUIRED)

set(LIB_NAME "ZipExtractor")

add_library(${LIB_NAME} STATIC extractZip.cpp)

target_include_directories(${LIB_NAME}
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE
		${ZLIB_INCLUDE_DIR}
)

target_link_libraries(${LIB_NAME}
	PRIVATE
		${ZLIB_LIBRARY}
		${SDL2_LIBRARIES}
)
