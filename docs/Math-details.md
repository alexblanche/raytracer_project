# Math details

Since this project's main goal is to have fun doing math and coding by hand, I did all the following calculations by myself, with minimum help from outside sources, and no AI assistance.  
Here are the details of the calculations used in this project.

## Object intersection

$(u \vert v)$ denotes the dot product of vectors $u$ and $v$.

### **Sphere**

We denote:
- $c$ the center of the sphere, $r$ its radius;
- $u$ the origin of the ray, $dir$ its direction (we assume $\vert dir\vert = 1$);
- $v$ the vector from the origin of the ray to the center of the sphere: $v = u - c$.

We have to solve the equation $\vert v - t \cdot dir \vert^2 = r^2$ for $t > 0$.  
The system is equivalent to:  

$$t^2 \cdot \vert dir\vert^2 - 2 \cdot (dir \vert v)\cdot t + \vert v\vert^2 - r^2 = 0$$
Or:  
$$t^2 - 2\cdot (dir \vert v)\cdot t + (\vert v\vert^2 - r^2) = 0$$

Let us denote $\delta = (dir \vert v)$.
The discriminant is: $\Delta = (4 \delta^2 - 4 (\vert v\vert^2 - r^2)) = 4 (\delta^2 - \vert v\vert^2 + r^2)$.  

If $\Delta < 0$, then the ray's direction does not intersect the sphere. Otherwise, the two possible solutions are $t_1 = \delta - \sqrt{\Delta}$ and $t_2 = \delta + \sqrt{\Delta}$.  

- If $t_1 \geq 0$, this means the ray originates from outside the sphere
and the sphere is in the way of the ray, thus **$t_1$ is the solution**;
- If $t_1 < 0$ and $t_2 \geq 0$, this means the ray originates from inside the sphere,
and **$t_2$ is the solution**;
- Otherwise, $t_1 < 0$ and $t_2 < 0$ means the sphere is behind the ray and is not hit. 

### **Plane**

We denote:
- $u$ the origin of the ray: $u = (u_x, u_y, u_z)$;
- $dir$ the direction of the ray: $dir = (d_x, d_y, d_z)$;
- $n$ the normal of the plane: $n = (a, b, c)$.  
The normal and the direction are supposed to be unit vectors ($\vert n\vert = \vert dir\vert = 1$).

The equation of the plane is given by its parameters $a, b, c, d$: a point $(x, y, z)$ belongs to the plane if $ax + by + cz + d = 0$.

We are searching for $t > 0$ such that $u + t \cdot dir$ belongs to the plane,
i.e. $a(u_x + t\cdot d_x) + b(u_y + t\cdot d_y) + c(u_z + t\cdot d_z) + d = 0$. We deduce the value of $t$:

$$t = -\ \frac{a u_x + b u_y + c u_z + d}{a d_x + b d_y + c d_z}$$

We can rewrite $t$ as:

$$t = -\ \frac{(n\ \vert\ u) + d}{(n\ \vert\ dir)}$$

There is no solution only if $(n \vert dir) = 0$, i.e. when the ray is parallel to the plane. If $t < 0$, the ray is pointing away from the plane.

### **Triangle, quad**

We can determine the distance $t$ to the plane containing the polygon in the same way as for planes, from the normal vector and the ``d`` value. We then need to check whether the intersection point lies within the polygon. Let us denote $p = u + t\cdot dir$, and $c = p - p_0$, where $p_0$ is the ``position`` of the polygon, and $u, dir$ are respectively the origin and direction of the ray.

For a triangle $(p_0, p_1, p_2)$ (with $v_1 = p_1 - p_0$ and $v_2 = p_2 - p_0$), we determine the barycentric coordinates of $c$ by solving for $l_1, l_2 \in [0, 1]$:

$$l_1 \cdot v_1 + l_2 \cdot v_2 = c$$

This gives us 3 equations (for $x, y, z$) on two variables. We use **Cramer's rule** to solve the equations $x, y$:

$$\left\lbrace
\begin{array}{rcl}
l_1 & = & \frac{1}{det_{xy}} (c_x  v_{2y} - c_y v_{2x}) \\
l_2 & = & \frac{1}{det_{xy}} (v_{1x} c_y  - v_{1y} c_x) \\
\end{array}
\right. , \text{ where } det_{xy} = v_{1x} v_{2y} - v_{1y} v_{2x}$$

If the determinant $det_{xy} = 0$ (which means their projections onto the plane $z = 0$ are collinear),
we solve the equations for $x,z$ or $y,z$, depending on which determinant is non-zero.   
If $l_1, l_2 \in [0, 1]$ and $l_1 + l_2 < 1$, then $c$ lies within the trianlge, and $t$ is our solution.

For a quad $(p_0, p_1, p_2, p_3)$, we check whether the point lies within the triangles $(p_0, p_1, p_2)$ as before, and if not in the triangle $(p_0, p_2, p_3)$.


### **Axis-aligned bounding box (AABB)**

*Axis-aligned bounding boxes* are boxes whose axes are $(1,0,0), (0,1,0), (0,0,1)$. Two implementations are proposed, one in which the ``position`` vector is the **center** of the box, and one in which it is the (lower) **corner** of the box. These boxes are only used (so far) as bounding boxes, and not as concrete objects.

- **Center**:
Let $c = (c_x, c_y, c_z)$ be the center of the box.
The ``dims`` vector contains the dimensions $(d_x, d_y, d_z)$ of the box: the box is defined as the region bounded by the six planes $(x = c_x \mp d_x), (y = c_y \mp d_y), (z = c_z \mp d_z)$ (so each term is half the length of the box along this axis).

To measure the distance, we first check whether the point lies inside the box, in which case we return $0$ (as we are not interested in the distance to the surface of a bounding box).
```C++
// Check whether u is inside the box
if (std::abs(v.x) <= dims.x && std::abs(v.y) <= dims.y && std::abs(v.z) <= dims.z)
    return 0.0_r;
```

If not, then the ray-box intersection lies within a face. We denote $v(t) = u + t\cdot dir$, where $u = (u_x, u_y, u_z)$ is the origin of the ray, $dir = (dir_x, dir_y, dir_z)$ is its direction, and $t > 0$.

Along an axis, say $x$, we only consider the face $(x = c_x - d_x)$ if $dir_x > 0$, and $(x = c_x + d_x)$ if $dir_x < 0$. We write $a = sign(dir_x) = \frac{d_x}{\vert d_x \vert}. The solution $t_x$ satisfies: $u_x + t_x \cdot dir_x = c_x - a d_x$, hence:

$$t_x = \frac{c_x - u_x - a d_x}{dir_x}$$

To see if this solution is admissible, we must check whether the obtained point $v(t_x)$ is located in the face:

$$-d_y \leq u_y + t_x \cdot dir_y - c_y \leq d_y \text{ and } -d_z \leq u_z + t_x \cdot dir_z - c_z \leq d_z$$

We observe that only one of $t_x, t_y, t_z$ can be admissible, thus we return the first that we find.


### **Box**

A box is defined by a *center* $c$ (the vector ``position``), three *axes* denoted $n_1, n_2, n_3$, and the *dimensions* denoted $l_1, l_2, l_3$ along the three axes respectively. The box is defined as the region bounded by the six planes (defined by (*position*, *normal*)) $(c + a l_1 n_1, a n_1)$, $(c + a l_2 n_2, a n_2)$, $(c + a l_3 n_3, a n_3)$, with $a \in \{ -1, 1\}$.

Let $M$ be the matrix whose columns are the axes $n_1, n_2, n_3$. This matrix represents the basis of the *box-space*. The matrix that transforms a world-space point $p$ into box-space is $M^{-1}$. Since the vectors $n_1, n_2, n_3$ are pairwise orthogonal and unit vectors, $M$ is an orthogonal matrix, and $M^{-1} = M^t$ (the transpose of $M$).

Given a ray $r = (u, dir)$, we consider the box-space ray $r_b$, with origin $u_b = M^{-1}(u - c)$ and direction $dir_b = M^{-1} dir$. In this space, the box is an *aabb*, centered at $c_b = (0, 0, 0)$, and we can compute the distance $t$ like we did in the previous section.

<!-- ### **Cylinder**

/* We denote the origin, direction and radius of the cylinder o, d and r,
       and the origin and direction of the ray. */

/* Step 1: check if the ray intersects the side of the cylinder

When p is a point in space, the projection of p on the line of vector d is
(o + s.d), where s is such that (p - (o + sd) | d) = 0,
i.e. (p - o | d) - s (d | d) = 0, hence s = (p - o | d)
When p = u + t dir, s = (u + t dir - o | d) = (u - o | d) + t (dir | d)

We search for t such that (u + t dir - (o + ((u-o|d) + t (dir|d)) d)).norm() = r,
so ((u - o - (u-o|d)d) + t (dir - (dir|d)d)).normsq() - r^2 = 0,
If we write A = (u - o - (u-o|d)d) = (Ax, Ay, Az) and B = (dir - (dir|d)d) = (Bx, By, Bz),
we can rewrite the expression as:
(A + t B).normsq() - r^2 = 0
(Ax + t Bx)^2 + (Ay + t By)^2 + (Az + t Bz)^2 - r^2 = 0,
Ax^2 + 2tAxBx + t^2 Bx^2 + ...(y)... + ...(z)... - r^2 = 0,
t^2(Bx^2 + By^2 + Bz^2) + 2t(AxBx + AyBy + AzBz) + (Ax^2 + Ay^2 + Az^2 - r^2) = 0,
t^2 * B.normsq() + 2t * (A|B) + (A.normsq() - r^2) = 0,

We solve for t: delta = 4(A|B)^2 - 4 * B.normsq() * (A.normsq() - r^2).
If delta >= 0, t = - (A|B) +- sqrt(delta/4) / B.normsq(),
t is a solution if (1) t >= 0, (2) 0 <= s <= length, i.e.
0 <= (u - o | d) + t (dir | d) <= length

The case analysis is detailed below.

/* Case analysis:
We compute s1 (associated with t1), s2 and whether we are outside the cylinder.
"s ok" means that 0 <= s <= length, i.e. the projection on the line is within the cylinder.

* if (s1 not ok) && (s2 not ok) ->
    if (s1 < 0 && s2 < 0 || s1 > length && s2 > length): return infinity (the ray misses the cylinder)
    else: continue (if outside, the ray goes through both edge disks, otherwise only one edge disk)
* if (s1 ok) && (s2 ok) -> return t2 (u is inside the cylinder)
* if (s1 not ok) && (s2 ok) ->
    if outside: continue (the edge disk is between u and the intersection at t2)
    else: return t2 (u is inside, so the intersection is at t2)
* if (s1 ok) && (s2 not ok) ->
    if outside: return infinity (u is outside and the cylinder is behind it)
    else: continue (the edge disk is between u and the intersection with the infinite cylinder at t2) 
*/

/* Step 2: if a solution has not been found, check the two edge disks

Now let us compute the intersection with the disk at the edge of the cylinder.
Its center is denoted v:
if we are outside the infinite cylinder,
    if (dir | d) >= 0, v = o,
    otherwise v = o + length * d
if we are inside the infinite cylinder,
    if (dir | d) >= 0, v = o + length * d,
    otherwise v = o

We compute t such that u + t dir belongs to the plane orthogonal with d and located at v:
(u + t dir - v | d) = 0,
t = (v - u | d) / (dir | d)
(if (dir|d) != 0, which we may assume, since otherwise we would have concluded at step 1)
t is a solution if t >= 0 and (u + t dir - v).normsq() <= r^2, which we may also assume.
*/ -->

<!-- ## Normal vector computation

### Sphere

const auto& [ _, up_dir, _ ] = o.matrix;
const rt::vector t = (up_dir ^ local_normal).unit();
const rt::vector b = t ^ local_normal;

return matprod(t, b, local_normal, tangent_space_normal);



### **Triangle, quad**

Computation of tangent space
//     v1 = x1 * t + y1 * b
//     v2 = x2 * t + y2 * b

//     In matrix form:
//     (v1.x v1.y v1.z)   (x1 y1)(t.x t.y t.z)
//     (v2.x v2.y v2.z) = (x2 y2)(b.x b.y b.z)

//     So,
//     (x1 y1)-1 (v1.x v1.y v1.z)   (t.x t.y t.z)
//     (x2 y2)   (v2.x v2.y v2.z) = (b.x b.y b.z)

//     (x1 y1)-1                             (y2  -y1)
//     (x2 y2)   = (1 / (x1 * y2 - x2 * y1)) (-x2  x1)

// Recompute the tangent space with Gram-Schmidt's method
const rt::vector t2 = (t - ((t | local_normal) * local_normal)).unit();
const rt::vector b2 = t2 ^ local_normal;

//return tangent_space_normal.x * t2 + tangent_space_normal.y * b2 + tangent_space_normal.z * local_normal;
return matprod(
    t2,             tangent_space_normal.x,
    b2,             tangent_space_normal.y,
    local_normal,   tangent_space_normal.z
); -->

## Sampling

### **Sphere sampling**

We want to uniformly sample a point on the surface of a sphere of radius $1$ centered on $(0, 0, 0)$. The point we want to sample has spherical coordinates $(1, \theta, \phi)$, with $\theta$ the angle around the $y$-axis, and $\phi$ the vertical angle ($phi = 0$ at the "north pole" with $y = 1$, and $phi = \frac{\pi}{2}$ on the "equator" with $y = 0$).

First, $\cos \phi$ is sampled uniformly on $[-1, 1)$, and $\sin \phi$ is deduced (as a positive value). In the code:
```C++
const real cos_phi = 2.0_r * rg.random_ratio() - 1.0_r;
const real sin_phi = sqrt(1.0_r - cos_phi * cos_phi);
```

Then, $theta$ is sampled uniformly on $[0, 2\pi)$.
```C++
const real theta = rg.random_angle(); // = (2 * PI) * rg.random_ratio();
```

The point obtained is thus:

$$p = \begin{pmatrix}
\cos\theta \cdot \sin\phi \\
\cos\phi \\
\sin\theta \cdot \sin\phi \\
\end{pmatrix}$$

This sampling strategy is equivalent to sampling a point $P_C$ uniformly on the circumscribed cylinder's surface (excluding the two bases), projecting $P_C$ onto the axis of the cylinder to obtain $P_O$ (axial projection), and taking the intersection $P$ of the segment $[P_O, P_C]$ on the sphere. This projection is a bijection, except at the poles, which have an area of 0.

To show that this method gives a uniform distribution, we will do a proof similar to Archimedes' _On the Sphere and Cylinder_ (225 BCE) and prove that the axial projection of any measurable region of the sphere onto the cylinder preserves area. Hence, if we sample uniformly on the cylinder's vertical surface and project the samples onto the sphere, the samples will be uniformly distributed on the sphere as well.

- An area differential $\mathrm{d}S$ on the sphere can be delimited by angles $(\theta, \theta + \mathrm{d}\theta)$ and $(\phi, \phi + \mathrm{d}\phi)$. The area of this region is approximately the same as a rectangle of length $\sin \phi \mathrm{d}\theta$ and height $\mathrm{d}\phi$: the area $A = \mathrm{d}\phi \cdot \sin \phi \mathrm{d}\theta + o(\mathrm{d}\theta\mathrm{d}\phi)$.  
- When projected onto the cylinder, the length $\sin\phi \mathrm{d}\theta$ gives a length $\mathrm{d}\theta$ and the height $\mathrm{d}\phi$ gives a height $\sin\phi \mathrm{d}\phi$ (as the projected rectangle makes an angle $\phi$ with the vertical surface of the cylinder). The area projected onto the cylinder is $A' = \mathrm{d}\theta \cdot \sin\phi \mathrm{d}\phi + o(\mathrm{d}\theta\mathrm{d}\phi) = A$.

Thus, by integrating over area differentials, any measurable region $S$ on the sphere has the same measure as the projected region on the cylinder.

### **Triangle, quad sampling**

We use the sampling strategy exposed in [PBRT](https://www.pbr-book.org/4ed/Shapes/Triangle_Meshes#Sampling). We sample a point uniformly in the unit square ( $x, y \sim \mathcal{U}(0, 1)$ ). The uv-coordinates in the triangle are obtained by the transformation:  

$$f(x, y) = \left\lbrace
\begin{array}{rl}
(x / 2, y - x / 2) & \text{if } x < y\\
(x - y / 2, y / 2) & \text{otherwise}\\
\end{array}
\right.$$

To show that this transformation preserves area, we can compute its jacobian:

$$f(x, y) =
\left(
\begin{matrix}
f_1(x, y) \\
f_2(x, y) \\
\end{matrix}
\right),
J_f =
\left(
\begin{matrix}
\frac{\partial f_1}{\partial x} & \frac{\partial f_1}{\partial y} \\
\frac{\partial f_2}{\partial x} & \frac{\partial f_2}{\partial y} \\
\end{matrix}
\right)$$

$$\frac{\partial f_1}{\partial x} = \left\lbrace
\begin{array}{rl}
1 / 2 & \text{if } x < y\\
1 & \text{otherwise}\\
\end{array}
\right.$$

$$\frac{\partial f_1}{\partial y} = \left\lbrace
\begin{array}{rl}
0 & \text{if } x < y\\
-1 / 2 & \text{otherwise}\\
\end{array}
\right.$$

$$\frac{\partial f_2}{\partial x} = \left\lbrace
\begin{array}{rl}
-1 / 2 & \text{ if } x < y\\
0 & \text{otherwise}\\
\end{array}
\right.$$

$$\frac{\partial f_2}{\partial y} = \left\lbrace
\begin{array}{rl}
1 & \text{ if } x < y\\
1 / 2 & \text{ otherwise}\\
\end{array}
\right.$$

$\det J_f = 1 / 2$ is a constant, hence the transformation preserves area.

<!-- ## Post-processing

(FFT Bloom, to do) -->