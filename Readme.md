# Raytracer project in C++

## Goal

The goal of this project is to code a path-tracer that handles all sorts of objects, including polygon meshes, and realistic shading, reflections, refraction and different kinds of materials. The secondary goal is to make it converge as fast as possible.

### State of the project

The project is an implementation of the backward path-tracing algorithm: for each pixel of the image, a ray is cast from the camera in the direction of the pixel, bounces off the objects in the scene until it reaches a source of light. A color is then calculated from the color and intensity of the light source, as well as the colors of the materials encountered at each bounce, and applied to the pixel. Multiple samples are computed for each pixel, and averaged out to produce the final image: the more samples, the less grain the final image will have. This method works well in the case of ambient light, but is extremely inefficient in the case of dark scenes or directional light sources.  

Current state:  
![Screen](pictures/porsche_street.jpg)  
![Screen](pictures/dragon_1000.jpg)  
![Screen](pictures/stool_HD_1000.jpg) 
![Screen](pictures/porsche_field.jpg)  
Models found at [free3d.com](https://free3d.com/fr/3d-model/wood-stool-303532.html) and [CGTrader.com](https://www.cgtrader.com/free-3d-models/car/sport-car/2016-porsche-911-turbo), background from [Poly Haven](https://polyhaven.com/).  

The program currently handles polygon meshes (composed of triangles and quads) and multiple shapes (triangles, quads, spheres, planes, boxes and cylinders), made up of materials of various reflectivity (from diffuse to glossy, to mirror-like), specular probability (to simulate realistic reflections on non-metallic materials) and refractive index (for water, glass). Surfaces can be textured with images read from bmp files, with normal maps, and objects can be imported from Wavefront .obj/.mtl files. The rendering of polygon meshes is accelerated with the [Bounding Volume Hierarchy](https://en.wikipedia.org/wiki/Bounding_volume_hierarchy) method. The background may be textured with a 360 image mapped onto a sphere at infinite distance. Scenes are defined in a file ```scene.txt``` at the root (see the syntax in the [User guide](User-guide.md)). The rendered images can be exported as raw data or as a .bmp file. The raw data files from multiple renders of the same scene can be merged into a .bmp file, or can be postprocessed to add a glowing effect around bright lights (see the [User guide](User-guide.md)).

Next steps:  
Future plans involve the introduction of some bidirectionality to the path-tracing (to accelerate the rendering of dark scenes) and a conversion to GPU rendering.


## How to run the code

This C++ project requires a C++17-compatible compiler and the [SDL2 library](https://www.libsdl.org/).

### Windows
To use SDL2 with MinGW-w64 on Windows, I downloaded the file ```SDL2-devel-2.28.5-mingw.zip``` from the [latest SDL2 release](https://github.com/libsdl-org/SDL/releases/tag/release-2.28.5), copied the folders ```include```, ```lib``` and the file ```bin/SDL2.dll``` (from the ```x86_64-w64-mingw32``` folder for 64-bit) in a folder ```sdl``` located at the root of my project.

<!-- Instructions for my older MinGW -->
<!-- To use the parallel render loop, I copied the ```include/parallel/parallel.h``` file from https://stackoverflow.com/a/49188371. Since the ```thread``` and ```mutex``` libraries were not recognized by my MinGW, I added the files ```mingw.thread.h```, ```mingw.mutex.h``` and ```mingw.invoke.h``` files from https://github.com/meganz/mingw-std-threads/tree/master in the ```include``` folder of my MinGW folder, and added the line ```#define _WIN32_WINNT 0x0501``` at the beginning of ```mingw.thread.h```. -->

To compile, create a folder ```build``` at the root of the project and copy the ```SDL2.dll``` file (previously copied in ```sdl/bin```) into it. Then I use the following command lines (you may have to specify your own paths to gcc and g++). This produces the executables ```main```, ```merge```, ```postprocess``` and ```legacy_raytracer``` (see the [User guide](User-guide.md)).
```
$ cmake .. -G "MSYS Makefiles" -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc -DCMAKE_MAKE_PROGRAM=make -DCMAKE_PREFIX_PATH=sdl  
$ make  
$ main.exe 5
```

### Linux
 
Install the SDL2 library, then create a folder ```build``` at the root, move to it and use the command lines:  
``````
$ cmake ..
$ make
$ ./main 5
``````

See command-line arguments and scene descriptor syntax in the [User guide](User-guide.md).  

## Sources

[_Ray Tracing in One Weekend_](https://raytracing.github.io/books/RayTracingInOneWeekend.html)  
[_Scratchapixel_](https://www.scratchapixel.com)  
[_Physically Based Rendering_](https://www.pbrt.org/)