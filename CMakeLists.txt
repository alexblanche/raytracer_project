cmake_minimum_required(VERSION 3.12)
project(raytracer VERSION "0.0.1")

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-g -Wall -Wextra -Werror -O3)
add_compile_options(-fdiagnostics-color)
add_compile_options(-march=native)
add_compile_options(-ffast-math)

set(SDL2_NO_MWINDOWS 1)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
# message("- SDL2 found: ${SDL2_FOUND}")
# message("- SDL2_INCLUDE_DIRS: ${SDL2_INCLUDE_DIRS}")
# message("- SDL2_LIBRARIES: ${SDL2_LIBRARIES}")

# -I include/
include_directories(include/)

# libmyraytracer.so
set (libname myraytracer)
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
	src/auxiliary/tracing.cpp
	src/scene/camera.cpp
	src/scene/scene.cpp
	src/file_readers/bmp_reader.cpp
)

target_link_libraries(${libname}
	${SDL2_LIBRARIES}
)

target_include_directories(${libname}
	PUBLIC
		${SDL2_INCLUDE_DIRS}
)

set(main_name main)
add_executable(${main_name}
	main.cpp
)
target_link_libraries(${main_name}
	${libname}
)