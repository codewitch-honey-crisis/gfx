#### [← Back to index](index.md)

<a name="6"></a>

# 6. Positioning

Positioning elements and getting metrics is a common utilitarian task in GFX and to that end several structures are provided to facilitate it. These structures include points, sizes, rectangles, and paths. All of the positioning and metric structures in GFX are templatizes such that you can declare for example, `pointx<int>` for a point with `int` `x` and `y` coordinates. However, GFX tends to use the `int16_t` and `uint16_t` instantiations of these, aliased as for example `spoint16` and `point16` respectively.

These structures are mutable, but in most cases their manipulation methods will return a new instance of type with new values in it. For example, `rect16`'s `crop()` method returns a new `rect16`.

The exception are methods that end with `_inplace`. Such methods modify an existing structure.

<a name="6.1"></a>

## 6.1 Points

A point is a single coordinate on a 2D cartesian plane. As above, typically in GFX, coordinates are expressed using 16-bit signed and unsigned integers. `spoint16` and `point16` are staples for this and represent a point with signed coordinates and a point with unsigned coordinates, respectively. Each point contains an `x` and `y` value as well as several methods for manipulating them, such as `offset()` as well as methods for doing computations on them, such as `intersects()`.

```cpp
point16 pt(5,7);
pt = pt.offset(-5,3);
// prints "pt is (0,10)"
printf("pt is (%d,%d)\r\n",(int)pt.x,(int)pt.y);
```

<a name="6.2"></a>

## 6.2 Sizes

A size is a set of 2D dimensions representing a rectangular area. Its members include `width` and `height` plus some utility methods, such as `bounds()` which returns a bounding rectangle anchored to (0,0). The common sizes are `ssize16` for a signed size, and `size16` for an unsigned size.

```cpp
size16 sz(10,10);
// prints "sz is (10,10)"
printf("sz is (%d,%d)\r\n",(int)sz.width,(int)sz.height);
```

<a name="6.3"></a>

## 6.3 Rectangles

Rectangles are where things start to get involved. The idea behind them remains simple - they are a two coordinate structure specifying the upper left and lower right points of a rectangular area. Despite this simplicity, there is a lot you can do with them, and they have an API to match. Their main members are `x1`, `y1`, `x2` and `y2` representing both points in the rectangle. The most common rectangle types are `srect16` and `rect16` for signed and unsigned rectangles respectively.

Rectangles can be cropped, inflated/deflated, offset, centered, flipped in orientation/normalized, and hit tested with points and other rectangles. Due to their flexibility, rectangles are often taken wherever two coordinates are needed, such as when drawing lines.

```cpp
// create a 10x10 rect
rect16 r(0,0,9,9);
// move the rect to (10,10)
r = r.offset(10,10); 
// prints 10
printf("width = %d\r\n",r.width());
```

<a name="6.4"></a>

## 6.4 Paths

Paths are a series of points usually used to determine the boundaries of a polygon. All points in a path are of the same type. An `spath16` consists of a `size()` and an array of that size of `spoint16` points that make up the path. For polygons, these should be in clockwise order. `path16` similarly, contains `point16` structures.

```cpp
// A 32x32 triangle via a path
spoint16 path_points[] = {spoint16(0,31),spoint16(15,0),spoint16(31,31)};
spath16 path(3,path_points);
// offset it so it starts at (10,20)
path.offset_inplace(10,20); 
```
It should be noted that unlike other structures, path manipulation members do not return a copy of the path. This is because doing so would require allocating memory - something GFX is loathe to do for you except as required, and usually temporarily. Avoiding casual allocations like this keeps heap fragmentation down.

[→ Streams](streams.md)

[← Drawing](drawing.md)

