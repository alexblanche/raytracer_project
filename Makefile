all:
	g++ -std=c++17 -Wall -I sdl/include -L sdl/lib -o main.exe \
		src/screen/color.cpp \
		src/scene/material/texture.cpp \
		src/scene/material/material.cpp \
		src/screen/image.cpp \
		src/screen/screen.cpp \
		src/auxiliary/randomgen.cpp \
		src/light/hit.cpp \
		src/light/ray.cpp \
		src/light/vector.cpp \
		src/scene/objects/object.cpp \
		src/scene/objects/plane.cpp \
		src/scene/objects/sphere.cpp \
		src/scene/objects/triangle.cpp \
		src/scene/objects/quad.cpp \
		src/scene/objects/box.cpp \
		src/scene/objects/bounding.cpp \
		src/scene/objects/cylinder.cpp \
		src/auxiliary/tracing.cpp \
		src/scene/camera.cpp \
		src/scene/scene.cpp \
		src/file_readers/bmp_reader.cpp \
		main.cpp -lmingw32 -lSDL2main -lSDL2