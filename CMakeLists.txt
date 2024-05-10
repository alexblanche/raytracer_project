cmake_minimum_required(VERSION 3.12)
project(raytracer VERSION "0.0.1")

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-g -Wall -Wextra -Werror -O3)
add_compile_options(-fdiagnostics-color)
add_compile_options(-march=native)
add_compile_options(-ffast-math)

# -I include/
include_directories(include/)

# Necessary to have the console be the stdout channel
set(SDL2_NO_MWINDOWS 1)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# libmyraytracer.so
set (libname raytracer)
add_library(${libname} SHARED
	src/screen/color.cpp
	src/scene/material/texture.cpp
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
	src/tracing/tracing.cpp
	src/scene/camera.cpp
	src/scene/scene.cpp
	src/file_readers/bmp_reader.cpp
	src/file_readers/raw_data.cpp
	src/auxiliary/clustering.cpp
	src/file_readers/obj_parser.cpp
	src/file_readers/mtl_parser.cpp
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

# Legacy raytracer (first version, 2014)
add_executable(legacy_raytracer
	src/screen/color.cpp
	src/screen/image.cpp
	src/screen/screen.cpp
	src/light/hit.cpp
	src/light/ray.cpp
	src/light/vector.cpp
	src/scene/objects/plane.cpp
	src/scene/objects/sphere.cpp
	src/legacy/source.cpp
	src/legacy/raytracing/application.cpp
	src/legacy/raytracing/tracing.cpp
	src/legacy/legacy.cpp
)