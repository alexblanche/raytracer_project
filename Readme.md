# Raytracer project in C++

## Goal

The primary goal is to revamp an old school project from 2014, and then to improve it.
The project was a rudimentary raytracer, which only handled spheres and planes, and notably did not handle multiple reflections.

### Original pictures

![Screen](https://github.com/alexblanche/raytracer_project/blob/main/pictures/rt1.png)

![Screen](https://github.com/alexblanche/raytracer_project/blob/main/pictures/rt2.png)

## How to run the code

This C++ project requires the [SDL2 library](https://www.libsdl.org/).
More precise instructions to be added.

Personally I downloaded the file ```SDL2-devel-2.28.5-mingw.zip``` from the [latest SDL2 release](https://github.com/libsdl-org/SDL/releases/tag/release-2.28.5), copied the folders ```include``` and ```lib``` in a folder ```sdl``` located at the root of the project, as well as the file ```bin/SDL2.dll``` at the root. I compile using the makefile, and I needed to add ```"${workspaceFolder}/sdl/include"``` to the includePath.