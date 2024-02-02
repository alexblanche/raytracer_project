all:
	g++ -std=c++17 -Wall -I sdl/include -L sdl/lib -o main.exe \
		src/screen/color.cpp \
		src/scene/material/material.cpp \
		src/screen/image.cpp \
		src/screen/screen.cpp \
		src/light/hit.cpp \
		src/light/ray.cpp \
		src/light/vector.cpp \
		src/scene/sources/source.cpp \
		src/scene/objects/object.cpp \
		src/scene/objects/plane.cpp \
		src/scene/objects/sphere.cpp \
		src/auxiliary/application.cpp \
		src/auxiliary/tracing.cpp \
		main.cpp -lmingw32 -lSDL2main -lSDL2