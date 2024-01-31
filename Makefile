all:
	g++ -std=c++17 -Wall -I sdl/include -L sdl/lib -o main.exe \
		src/screen/color.cpp \
		src/screen/image.cpp \
		src/screen/screen.cpp \
		src/light/hit.cpp \
		src/light/ray.cpp \
		src/light/vector.cpp \
		src/light/light.cpp \
		src/objects/object.cpp \
		src/objects/plane.cpp \
		src/objects/sphere.cpp \
		main.cpp -lmingw32 -lSDL2main -lSDL2