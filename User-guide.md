# Raytracer project - User guide
1. [Syntax of the scene descriptor file](#syntax)
2. [Command-line arguments](#command)
3. [Merger executable](#merger)
4. [Postprocessing](#post)

## Syntax of the scene descriptor file <a name="syntax"></a>

The scene to render is defined in the file ```scene.txt```, with the following syntax.

### Initial parameters

The parameters should be proved in order, without comments.  

The resolution of the rendered image is defined by:  
``resolution width:1366 height:768``

The view angle of the camera is defined by the position of the camera in world space, the direction it points at, and a vector rightdir orthogonal to the direction and "to the right" (it is used to tilt the camera in any angle). The ``fov_width`` parameter designates the width of the screen in world space (used to zoom in or out of the scene). The associated parameter ``fov_height`` is defined automatically (for width/height aspect ratio). Finally, the ``distance`` parameter indicates the distance in world space between the screen and the camera.  
``camera position:(0, 0, 0) direction:(0, 0, 1) rightdir:(1, 0, 0) fov_width:1000 distance:400``  

To enable the depth of field camera effect, the focal distance and aperture can be specified with the commands:  
``focal_distance:500 aperture:100``  
on the same line as ``camera``.

The background color (the "sky") is defined by:  
``background_color 190 235 255``

When polygon meshes are used (see below), the rendering can be accelerated with the [Bounding Volume Hierarchy](https://en.wikipedia.org/wiki/Bounding_volume_hierarchy) method. The polygons are placed in boxes, which are themselves recursively enclosed in larger boxes, until only one box remains. The ``polygons_per_bounding`` parameter designates the number of polygons in terminal nodes, and its optimal value for peak performance depends on the object.  
To enable the BVH strategy, a non-zero value should be specified:  
``polygons_per_bounding 60``

Specifying 0 will disable the method, and a linear search will be performed instead:  
``polygons_per_bounding 0``


### Material definition

Materials are defined by the following parameters:
- ``color``: the base color of the material. (it will be covered by the texture in the case of a textured polygon)
- ``emitted_color``: when the material emits some light, the color to be emitted
- ``reflectivity``: the reflectivity of the material. 0 for purely diffuse (matte) materials, 1 for pure mirrors, and a value between 0 and 1 for glossy materials (with blurry reflections)
- ``emission``: the brightness of the material. 0 for materials that do not emit light. The value can go above 1 and to an arbitrary value, but note that the material will appear closer to white (but will still cast a light of the given ``emitted_color``).
- ``specular_p``: represents how reflective the material is. A value of 0 will have the reflections on the material be purely diffuse (for matte objects). With 1, the material will specularly reflect the entirety of the incoming light (with glossiness subject to the ``reflectivity`` parameter). A value between 0 and 1 will be a mix of ``color`` and the reflection of the incoming light.
- ``reflects_color``: when the parameter ``specular_p`` is different from 0, the specular reflections will keep the ``color`` of the material. Usually ``reflects_color`` is set at ``false``, except in the case of conductors and metals.
- ``transparency``: the amount of light transmitted through the material when going at a normal angle, for transparent materials. The amount of light transmitted depends on the angle,, so even will a value of 1, some reflections will occur when the angle between the ray and the normal approaches $\pi$/2, according to Fresnel's formula.
- ``scattering``: the scattering of the light when transmitted through the material, similarly to ``reflectivity`` for reflections. The value should be close to 0 for most transparent materials.
- ``refraction_index``: the refraction index of the material, used when the ray is be refracted according to Snell-Descartes' law. Air has 1, water 1.33, glass 1.52, and diamond 2.42.  

Materials are thus defined with this syntax:  
``material:(color:(120, 120, 120) emitted_color:(0, 0, 0) reflectivity:1 emission:0 specular_p:1.0 reflects_color:false transparency:0 scattering:0 refraction_index:1)``  

A material can be given a name with this syntax:  
``material m1 (color:(...) ...)``

### Object definition

Objects can be defined in any order, with the following syntax.

- Sphere
A sphere is defined by its center and radius.  
``sphere center:(-500, 0, 600) radius:120 material:m1``

- Plane
A plane is defined by a normal vector and a point the plane goes through.  
``plane normal:(0, -1, 0) position:(0, 160, 0) material:m1``

- Box
A box is defined by its center, two axes x and y (the z axis is the cross product of x and y), and three lengths: the "length" along the x axis, the "height" along the y axis, and the "depth" along the z axis. The axes do not need to be unit vectors.    
``box center:(166, -200, 600) x_axis:(100, 100, -100) y_axis:(-200, 100, -100) 300 200 300 material:m1``

- Polygons
Polygons can be either triangles or quads, and are defined by three or four points.  
``triangle (-620, -100, 600) (-520,100,500) (-540, -200, 700) material:m1``  
or  
``quad (-620, -100, 600) (-520, 100, 600) (-540, -200, 600) (-500, -250, 600) material:m1``

The polygons are one-sided, and should be declared in counter-clockwise order. Behavior when looking through the back side is undefined.

- Cylinder
A cylinder is defined by its origin, direction vector, radius and length. The origin is the center of the bottom disk. The radius parameter designates the common radius of the bottom and top disks. The length is the length of the cylinder along the direction vector. The direction vector does not need to be a unit vector.    
``cylinder origin:(0, 0, 0) direction:(1, -1, 1) radius:100 length:300 material:m1``


### Texture definition
Polygons support texturing, but textures mapped on a quad warp when the shape of the quad is different from the quad's UV-coordinates in texture space (e.g. when the UV-coordinates describe a rectangle in texture space, but the quad is a trapezoid in world space).  

Textures must be loaded from a bmp file, and given a name:  
``load_texture t1 file_name.bmp``

Then when an polygon is defined, we can specify the texture mapping with this syntax:    
``triangle (...) (...) (...) material:(color:(...) ...) texture:(t1 (0.2, 0.8) (0.5, 0.15) (0.7, 0.65))``   
or  
``quad (...) (...) (...) (...) material:m1 texture:(t1 (0.2, 0.8) (0.2, 0.15) (0.7, 0.15) (0.7, 0.8))``

The texture's name is followed by 3 (for a triangle) or 4 points (for a quad) in 2D, representing the UV-coordinates of the polygon in texture space.


### Polygon mesh import
The project supports polygon meshes in .obj format. Some features are not yet supported, like mtl files. Materials must be declared beforehand, with the same name as in the associated mtl file.  
An .obj file can be imported by specifying its file name, the texture mapped onto the object, a shifting vector and a scaling factor. The texture shall be declared beforehand and given a variable name.  
``````
material wood_mat (...)  // appearing in wooden_table.mtl
material metal_mat (...) // appearing in wooden_table.mtl
load_texture wood wood_texture.bmp
load_obj wooden_table.bmp (texture:wood shift:(1,0,0) scale:2)
``````

### Comments
A line can be commented by adding a ``#`` and a space at the beginning of the line:  
``# sphere [...]``




## Command line arguments <a name="command"></a>

The first command-line argument is always an integer, and specifies the maximum number of bounces allowed for each ray:  
``./main 10``  

The executable works in two modes: the interactive mode displays the image in a window every 10 samples per pixel, and the non-interactive mode generates a given number of samples per pixel without display.

### Interactive mode

The interactive mode is activated by default, with this syntax:  
``./main 10``  

The time taken to render each sample per pixel can be displayed with the option:   
``./main 10 -time``

When a single sample per pixel takes a long time, the progress and estimated time can be displayed with the option:  
``./main 10 -time all``

When the window appears, the render stops and waits for an input by the user:  
- Space/Enter: Continue
- B key: Save the generated image as ``image.bmp`` and continue
- R key: Save the generated raw data as ``image.rtdata`` and continue
- Esc key: Exit

### Non-interactive mode

To render in the non-interactive mode, the desired number of samples of pixels needs to be specified with this syntax:  
``./main 10 -rays 100``

The progress will be displayed and updated every 10 samples per pixel.


## Merger executable <a name="merger"></a>

The ``merge`` executable can be compiled with ```make merger```. It is used to merge several raw data files into one raw data file and a bmp file.
The syntax is the following:  
``./merge dest.bmp dest.rtdata source1.rtdata source2.rtdata ... sourcen.rtdata``  
When the source files are rtdata files ``source1.rtdata``, ..., ``sourcen.rtdata`` and the destination files are ``dest.bmp`` and ``dest.rtdata``.

## Postprocessing <a name="post"></a>

The postprocessing glow effect can be applied to raw data files with the ``postprocess`` executable, which can be compiled with ``make postprocess``. It generates an output bmp image.
The syntax is the following:  
``./postprocess source.rtdata dest.bmp``  
When the source file is ``source.rtdata`` and the destination file is ``dest.bmp``.