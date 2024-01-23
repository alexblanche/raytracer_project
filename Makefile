all:
	g++ -I sdl/include -L sdl/lib -o main.exe main.cpp -lmingw32 -lSDL2main -lSDL2