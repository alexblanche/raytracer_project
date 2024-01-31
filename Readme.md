# Raytracer project in C++

## Goal

The primary goal is to revamp an old school project from 2014, and then to improve it.  
The project was a rudimentary raytracer, which only handled spheres and planes, and notably did not handle multiple reflections.

### State of the project

The original raytracer was successfully restored and converted to SDL2.  
Next steps: scene description parsing, finite planes (triangles), then reflections, refraction, light halo, texturing...

Current state:

![Screen](https://github.com/alexblanche/raytracer_project/blob/main/pictures/rt3.jpg)

![Screen](https://github.com/alexblanche/raytracer_project/blob/main/pictures/rt2.png)

## How to run the code

This C++ project requires the [SDL2 library](https://www.libsdl.org/).  
Personally I downloaded the file ```SDL2-devel-2.28.5-mingw.zip``` from the [latest SDL2 release](https://github.com/libsdl-org/SDL/releases/tag/release-2.28.5), copied the folders ```include``` and ```lib``` in a folder ```sdl``` located at the root of the project, as well as the file ```bin/SDL2.dll``` at the root. I compile using the makefile, and I needed to add ```"${workspaceFolder}/sdl/include"``` to the includePath.

To use the parallel render loop, I copied the ```parallel/parallel.h``` file from https://stackoverflow.com/a/49188371. Since the ```thread``` and ```mutex``` libraries were not recognized by my MinGW, I added the files ```mingw.thread.h```, ```mingw.mutex.h``` and ```mingw.invoke.h``` files from https://github.com/meganz/mingw-std-threads/tree/master in the ```include folder``` of my MinGW folder, and added the line ```#define _WIN32_WINNT 0x0501``` at the beginning of ```mingw.thread.h```.