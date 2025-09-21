cmake_minimum_required(VERSION 3.12)
project(raytracer VERSION "0.0.1")

# SET (CMAKE_CXX_COMPILER "/usr/bin/g++")

set(CMAKE_CXX_STANDARD 23)
add_compile_options(-std=c++23)
add_compile_options(-Wall -Wextra -Werror -O3)
add_compile_options(-fdiagnostics-color)
add_compile_options(-march=native)
add_compile_options(-ffast-math)

include_directories(include/)

# Necessary to have the console be the stdout channel
set(SDL2_NO_MWINDOWS 1)

find_package(SDL2 REQUIRED)
# message("SDL2_INCLUDE_DIRS : ${SDL2_INCLUDE_DIRS}")
# message("SDL2_LIBRARIES : ${SDL2_LIBRARIES}")
include_directories(${SDL2_INCLUDE_DIRS})

##################################################################
### Main path-tracer
##################################################################

# libmyraytracer.so
set (libname raytracer)
add_library(${libname} SHARED
	src/screen/color.cpp
	src/scene/material/texture_info.cpp
	src/scene/material/texture.cpp
	src/scene/material/normal_map.cpp
	src/scene/material/background.cpp
	src/scene/material/material.cpp
	src/screen/image.cpp
	src/screen/screen.cpp
	src/auxiliary/randomgen.cpp
	src/light/hit.cpp
	src/light/ray.cpp
	src/light/vector.cpp
	src/scene/objects/object.cpp
	src/scene/objects/plane.cpp
	src/scene/objects/sphere.cpp
	src/scene/objects/triangle.cpp
	src/scene/objects/quad.cpp
	src/scene/objects/box.cpp
	src/scene/objects/bounding.cpp
	src/scene/objects/cylinder.cpp
	src/tracing/directions.cpp
	src/scene/light_sources/infinite_area.cpp
	src/tracing/tracing.cpp
	src/scene/camera.cpp
	src/file_readers/scene_parser.cpp
	src/scene/scene.cpp
	src/file_readers/bmp_reader.cpp
	src/file_readers/hdr_reader.cpp
	src/file_readers/normal_map_reader.cpp
	src/file_readers/raw_data.cpp
	src/auxiliary/octree.cpp
	src/auxiliary/clustering.cpp
	src/file_readers/obj_parser.cpp
	src/file_readers/mtl_parser.cpp
	src/tracing/multisample.cpp
	src/render/render_loops.cpp
)

target_link_libraries(${libname}
	${SDL2_LIBRARIES}
)

target_include_directories(${libname}
	PUBLIC
		${SDL2_INCLUDE_DIRS}
)

# Main executable
set(main_name main)
add_executable(${main_name}
	src/main.cpp
)
target_link_libraries(${main_name}
	${libname}
)

##################################################################
### Merger and postprocess
##################################################################


# Executable that combines an array of raw data files into a bmp file
set(merger_name merge)
add_executable(${merger_name}
	src/screen/color.cpp
	src/file_readers/raw_data.cpp
	src/file_readers/bmp_reader.cpp
	src/file_readers/merger.cpp
)

# Executable that applies glow postprocessing to a raw data file and generates a bmp file
set(glow postprocess)
add_executable(${glow}
	src/screen/color.cpp
	src/file_readers/raw_data.cpp
	src/file_readers/bmp_reader.cpp
	src/postprocess/glow.cpp
)

##################################################################
### Legacy raytracer
##################################################################

# Legacy raytracer (first version, 2014)
set(legacy legacy_raytracer)
add_executable(${legacy}
	src/screen/color.cpp
	src/screen/image.cpp
	src/screen/screen.cpp
	src/light/vector.cpp
	src/light/ray.cpp
	src/legacy/light/hit.cpp
	src/legacy/objects/object.cpp
	src/legacy/objects/plane.cpp
	src/legacy/objects/sphere.cpp
	src/legacy/source.cpp
	src/legacy/raytracing/application.cpp
	src/legacy/raytracing/tracing.cpp
	src/legacy/legacy.cpp
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

add_executable(sky
	src/screen/color.cpp
	src/file_readers/bmp_reader.cpp
	src/file_readers/hdr_reader.cpp
	src/screen/image.cpp
	src/screen/screen.cpp
	src/light/vector.cpp
	src/sky/sky_render.cpp
	src/sky/test_sky.cpp
)
target_link_libraries(sky
	${SDL2_LIBRARIES}
)
target_include_directories(sky
	PUBLIC
		${SDL2_INCLUDE_DIRS}
)