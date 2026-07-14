cmake_minimum_required(VERSION 4.0)
project(raytracer VERSION "0.8.1")

set(CMAKE_BUILD_TYPE "Release")

add_compile_options(-std=c++26)
add_compile_options(-Wall -Wextra -Werror -Wpedantic -pedantic-errors)
add_compile_options(-fdiagnostics-color)
add_compile_options(-O3 -ffast-math -march=native)

include_directories(include/)

# Necessary to have the console be the stdout channel
set(SDL2_NO_MWINDOWS 1)

find_package(SDL2 REQUIRED)
# message("SDL2_INCLUDE_DIRS: ${SDL2_INCLUDE_DIRS}")
# message("SDL2_LIBRARIES: ${SDL2_LIBRARIES}")
include_directories(${SDL2_INCLUDE_DIRS})

# if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
# 	message("Clang")
# elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
# 	message("GNU")
# endif()

##################################################################
### Source bundles
##################################################################

set(MATRIX_SOURCES
	src/screen/color.cpp
	src/image/matrix.cpp
)

set(IMAGE_READERS_SOURCES
	src/file_readers/image_files/bmp_reader.cpp
	src/file_readers/image_files/hdr_reader.cpp
	src/file_readers/image_files/raw_data.cpp
)

set(IMAGE_SOURCES
	${MATRIX_SOURCES}
	${IMAGE_READERS_SOURCES}
	src/screen/screen.cpp
	src/parallel/parallel.cpp
)

set(PARSERS_SOURCES
	src/file_readers/parsers/scene_parser.cpp
	src/file_readers/parsers/obj_parser.cpp
	src/file_readers/parsers/mtl_parser.cpp
	src/file_readers/image_files/normal_map_reader.cpp
)

set(OBJECTS_SOURCES
	src/scene/objects/plane.cpp
	src/scene/objects/sphere.cpp
	src/scene/objects/triangle.cpp
	src/scene/objects/quad.cpp
	src/scene/objects/box.cpp
	src/scene/objects/cylinder.cpp
)

set(MATERIAL_SOURCES
	src/scene/material/texture_info.cpp
	src/scene/material/texture.cpp
	src/scene/material/normal_map.cpp
	src/scene/material/background.cpp
	src/scene/material/material.cpp
)

set(SCENE_SOURCES
	${MATERIAL_SOURCES}
	${OBJECTS_SOURCES}
	src/scene/light_sources/infinite_area.cpp
	src/scene/camera.cpp
	src/scene/scene.cpp
)

set(TRACING_SOURCES
	src/tracing/direction.cpp
	src/tracing/tracing.cpp
	#src/tracing/multisample.cpp
	src/render/render_loops.cpp
)

set(AUXILIARY_ALGORITHMS_SOURCES
	src/accelerating_structures/octree.cpp
	src/accelerating_structures/clustering.cpp
	src/scene/bounding/aabb.cpp
)

set(MENU_SOURCES
	src/main_menu/file_handler.cpp
	src/main_menu/menu.cpp
)

##################################################################
### Main path-tracer
##################################################################

# Main executable
set(main_name main)
add_executable(${main_name}
	${IMAGE_SOURCES}
	${SCENE_SOURCES}
	${PARSERS_SOURCES}
	${TRACING_SOURCES}
	${AUXILIARY_ALGORITHMS_SOURCES}
	${MENU_SOURCES}
	src/parallel/parallel.cpp
	src/main.cpp
)

target_link_libraries(${main_name}
	${SDL2_LIBRARIES}
)

target_include_directories(${main_name}
	PUBLIC
		${SDL2_INCLUDE_DIRS}
)
##################################################################
### Merger and postprocess
##################################################################


# Executable that combines an array of raw data files into a bmp file
set(merger_name merge)
add_executable(${merger_name}
	${MATRIX_SOURCES}
	${IMAGE_READERS_SOURCES}
	src/file_readers/merger.cpp
	src/parallel/parallel.cpp
)


##################################################################
### Legacy raytracer
##################################################################

# Legacy raytracer (first version, 2014)
set(legacy legacy_raytracer)
add_executable(${legacy}
	${IMAGE_SOURCES}
	src/other/legacy/objects/legacy_object.cpp
	src/other/legacy/objects/legacy_plane.cpp
	src/other/legacy/objects/legacy_sphere.cpp
	src/other/legacy/source/legacy_source.cpp
	src/other/legacy/raytracing/legacy_tracing.cpp
	src/other/legacy/legacy.cpp
	src/parallel/parallel.cpp
)

target_link_libraries(${legacy}
	${SDL2_LIBRARIES}
)

target_include_directories(${legacy}
	PUBLIC
		${SDL2_INCLUDE_DIRS}
)

##################################################################
### Test sky dome
##################################################################

set(sky_name sky)
add_executable(${sky_name}
	src/other/sky/bmp_reader.cpp
	src/other/sky/screen.cpp
	src/parallel/parallel.cpp
	src/other/sky/sky_render_parallel.cpp
	src/other/sky/test_sky.cpp
)
target_link_libraries(${sky_name}
	${SDL2_LIBRARIES}
)
target_include_directories(${sky_name}
	PUBLIC
		${SDL2_INCLUDE_DIRS}
)

##################################################################
### Tests
##################################################################

set(test_reader_name testreader)
add_executable(${test_reader_name}
	${IMAGE_SOURCES}
	${PARSERS_SOURCES}
	${AUXILIARY_ALGORITHMS_SOURCES}
	${SCENE_SOURCES}
	src/tests/test_reader.cpp
)
target_link_libraries(${test_reader_name}
	${SDL2_LIBRARIES}
)
target_include_directories(${test_reader_name}
	PUBLIC
		${SDL2_INCLUDE_DIRS}
)

set(test_infinite_name testinfinite)
add_executable(${test_infinite_name}
	src/file_readers/image_files/hdr_reader.cpp
	src/parallel/parallel.cpp
	src/screen/screen.cpp
	src/scene/light_sources/infinite_area.cpp
	src/tests/test_infinite_area.cpp
)
target_link_libraries(${test_infinite_name}
	${SDL2_LIBRARIES}
)
target_include_directories(${test_infinite_name}
	PUBLIC
		${SDL2_INCLUDE_DIRS}
)

add_executable(testfile 	src/tests/test_file.cpp)
add_executable(testalloc 	src/tests/test_alloc.cpp)
add_executable(tests		src/tests/test.cpp)
