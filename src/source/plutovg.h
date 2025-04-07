/*
 * Copyright (c) 2020-2024 Samuel Ugochukwu <sammycageagle@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#ifndef PLUTOVG_H
#define PLUTOVG_H
#include <gfx_core.hpp>
#include <gfx_encoding.hpp>
#include <gfx_pixel.hpp>
#include <gfx_palette.hpp>
#include <gfx_positioning.hpp>
#include <gfx_vector_core.hpp>

#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(PLUTOVG_BUILD_STATIC) && (defined(_WIN32) || defined(__CYGWIN__))
#define PLUTOVG_EXPORT __declspec(dllexport)
#define PLUTOVG_IMPORT __declspec(dllimport)
#elif defined(__GNUC__) && (__GNUC__ >= 4)
#define PLUTOVG_EXPORT __attribute__((__visibility__("default")))
#define PLUTOVG_IMPORT
#else
#define PLUTOVG_EXPORT
#define PLUTOVG_IMPORT
#endif

// #ifdef PLUTOVG_BUILD
// #define PLUTOVG_API PLUTOVG_EXPORT
// #else
// #define PLUTOVG_API PLUTOVG_IMPORT
// #endif

#define PLUTOVG_API

#define PLUTOVG_VERSION_MAJOR 0
#define PLUTOVG_VERSION_MINOR 0
#define PLUTOVG_VERSION_MICRO 1

#define PLUTOVG_VERSION_ENCODE(major, minor, micro) (((major) * 10000) + ((minor) * 100) + ((micro) * 1))
#define PLUTOVG_VERSION PLUTOVG_VERSION_ENCODE(PLUTOVG_VERSION_MAJOR, PLUTOVG_VERSION_MINOR, PLUTOVG_VERSION_MICRO)

#define PLUTOVG_VERSION_XSTRINGIZE(major, minor, micro) #major"."#minor"."#micro
#define PLUTOVG_VERSION_STRINGIZE(major, minor, micro) PLUTOVG_VERSION_XSTRINGIZE(major, minor, micro)
#define PLUTOVG_VERSION_STRING PLUTOVG_VERSION_STRINGIZE(PLUTOVG_VERSION_MAJOR, PLUTOVG_VERSION_MINOR, PLUTOVG_VERSION_MICRO)

/**
 * @brief Gets the version of the plutovg library.
 * @return An integer representing the version of the plutovg library.
 */
PLUTOVG_API int plutovg_version(void);

/**
 * @brief Gets the version of the plutovg library as a string.
 * @return A string representing the version of the plutovg library.
 */
PLUTOVG_API const char* plutovg_version_string(void);

/**
 * @brief A function pointer type for a cleanup callback.
 * @param closure A pointer to the resource to be cleaned up.
 */
typedef void (*plutovg_destroy_func_t)(void* closure);

/**
 * @brief A function pointer type for a write callback.
 * @param closure A pointer to user-defined data or context.
 * @param data A pointer to the data to be written.
 * @param size The size of the data in bytes.
 */
typedef void (*plutovg_write_func_t)(void* closure, void* data, int size);

typedef ::gfx::gfx_result(*plutovg_write_callback_t)(const ::gfx::rect16& bounds, ::gfx::vector_pixel color, void* state);
typedef ::gfx::gfx_result(*plutovg_read_callback_t)(::gfx::point16 location, ::gfx::vector_pixel* out_color, void* state);

#define PLUTOVG_PI      3.14159265358979323846f
#define PLUTOVG_TWO_PI  6.28318530717958647693f
#define PLUTOVG_HALF_PI 1.57079632679489661923f
#define PLUTOVG_SQRT2   1.41421356237309504880f
#define PLUTOVG_KAPPA   0.55228474983079339840f

#define PLUTOVG_DEG2RAD(x) ((x) * (PLUTOVG_PI / 180.0f))
#define PLUTOVG_RAD2DEG(x) ((x) * (180.0f / PLUTOVG_PI))

#define PLUTOVG_MAKE_POINT(x, y) ::gfx::pointf{x, y)

/**
 * @brief A structure representing a rectangle in 2D space.
 */
typedef struct plutovg_rect {
    float x; ///< The x-coordinate of the top-left corner of the rectangle.
    float y; ///< The y-coordinate of the top-left corner of the rectangle.
    float w; ///< The width of the rectangle.
    float h; ///< The height of the rectangle.
    inline plutovg_rect(float x, float y, float w, float h) :
        x(x), y(y), w(w), h(h) {
    }
    inline plutovg_rect() {}
} plutovg_rect_t;

#define PLUTOVG_MAKE_RECT(x, y, w, h) (plutovg_rect_t(x, y, w, h))

/**
 * @brief plutovg_matrix_parse
 * @param matrix
 * @param data
 * @param length
 * @return
 */
PLUTOVG_API bool plutovg_matrix_parse(::gfx::matrix* matrix, const char* data, int length);

/**
 * @brief Represents a 2D path for drawing operations.
 */
typedef struct plutovg_path plutovg_path_t;

/**
 * @brief Enumeration defining path commands.
 */
typedef enum plutovg_path_command {
    PLUTOVG_PATH_COMMAND_MOVE_TO, ///< Moves the current point to a new position.
    PLUTOVG_PATH_COMMAND_LINE_TO, ///< Draws a straight line to a new point.
    PLUTOVG_PATH_COMMAND_CUBIC_TO, ///< Draws a cubic Bézier curve to a new point.
    PLUTOVG_PATH_COMMAND_CLOSE ///< Closes the current path by drawing a line to the starting point.
} plutovg_path_command_t;

/**
 * @brief Union representing a path element.
 *
 * A path element can be a command with a length or a coordinate point.
 * Each command type in the path element array is followed by a specific number of points:
 * - `PLUTOVG_PATH_COMMAND_MOVE_TO`: 1 point
 * - `PLUTOVG_PATH_COMMAND_LINE_TO`: 1 point
 * - `PLUTOVG_PATH_COMMAND_CUBIC_TO`: 3 points
 * - `PLUTOVG_PATH_COMMAND_CLOSE`: 1 point
 *
 * @example
 * const plutovg_path_element_t* elements;
 * int count = plutovg_path_get_elements(path, &elements);
 * for(int i = 0; i < count; i += elements[i].header.length) {
 *     plutovg_path_command_t command = elements[i].header.command;
 *     switch(command) {
 *     case PLUTOVG_PATH_COMMAND_MOVE_TO:
 *         printf("MoveTo: %g %g\n", elements[i + 1].point.x, elements[i + 1].point.y);
 *         break;
 *     case PLUTOVG_PATH_COMMAND_LINE_TO:
 *         printf("LineTo: %g %g\n", elements[i + 1].point.x, elements[i + 1].point.y);
 *         break;
 *     case PLUTOVG_PATH_COMMAND_CUBIC_TO:
 *         printf("CubicTo: %g %g %g %g %g %g\n",
 *                elements[i + 1].point.x, elements[i + 1].point.y,
 *                elements[i + 2].point.x, elements[i + 2].point.y,
 *                elements[i + 3].point.x, elements[i + 3].point.y);
 *         break;
 *     case PLUTOVG_PATH_COMMAND_CLOSE:
 *         printf("Close: %g %g\n", elements[i + 1].point.x, elements[i + 1].point.y);
 *         break;
 *     }
 * }
 */
typedef union plutovg_path_element {
    struct {
        plutovg_path_command_t command; ///< The path command.
        int length; ///< Number of elements including the header.
    } header; ///< Header for path commands.
    ::gfx::pointf point; ///< A coordinate point in the path.
} plutovg_path_element_t;

/**
 * @brief Iterator for traversing path elements in a path.
 */
typedef struct plutovg_path_iterator {
    const plutovg_path_element_t* elements; ///< Pointer to the array of path elements.
    int size; ///< Total number of elements in the array.
    int index; ///< Current position in the array.
} plutovg_path_iterator_t;

/**
 * @brief Initializes a path iterator for a given path.
 *
 * @param it The path iterator to initialize.
 * @param path The path to iterate over.
 */
PLUTOVG_API void plutovg_path_iterator_init(plutovg_path_iterator_t* it, const plutovg_path_t* path);

/**
 * @brief Checks if there are more elements to iterate over.
 *
 * @param it The path iterator.
 * @return `true` if there are more elements; otherwise, `false`.
 */
PLUTOVG_API bool plutovg_path_iterator_has_next(const plutovg_path_iterator_t* it);

/**
 * @brief Retrieves the current command and its associated points, then advances the iterator.
 *
 * @param it The path iterator.
 * @param points An array to store the points for the current command.
 * @return The path command for the current element.
 */
PLUTOVG_API plutovg_path_command_t plutovg_path_iterator_next(plutovg_path_iterator_t* it, ::gfx::pointf points[3]);

/**
 * @brief Creates a new path object.
 *
 * @return A pointer to the newly created path object.
 */
PLUTOVG_API plutovg_path_t* plutovg_path_create(void*(*allocator)(size_t), void*(*reallocator)(void*,size_t), void(*deallocator)(void*));

/**
 * @brief Increases the reference count of a path object.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @return A pointer to the same `plutovg_path_t` object.
 */
PLUTOVG_API plutovg_path_t* plutovg_path_reference(plutovg_path_t* path);

/**
 * @brief Decreases the reference count of a path object.
 *
 * This function decrements the reference count of the given path object. If
 * the reference count reaches zero, the path object is destroyed and its
 * resources are freed.
 *
 * @param path A pointer to the `plutovg_path_t` object.
 */
PLUTOVG_API void plutovg_path_destroy(plutovg_path_t* path,void(*deallocator)(void*));

/**
 * @brief Retrieves the reference count of a path object.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @return The current reference count of the path object.
 */
PLUTOVG_API int plutovg_path_get_reference_count(const plutovg_path_t* path);

/**
 * @brief Retrieves the elements of a path.
 *
 * Provides access to the array of path elements.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param elements A pointer to a pointer that will be set to the array of path elements.
 * @return The number of elements in the path.
 */
PLUTOVG_API int plutovg_path_get_elements(const plutovg_path_t* path, const plutovg_path_element_t** elements);

/**
 * @brief Moves the current point to a new position.
 *
 * This function moves the current point to the specified coordinates without
 * drawing a line. This is equivalent to the `M` command in SVG path syntax.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param x The x-coordinate of the new position.
 * @param y The y-coordinate of the new position.
 */
PLUTOVG_API bool plutovg_path_move_to(plutovg_path_t* path, float x, float y);

/**
 * @brief Adds a straight line segment to the path.
 *
 * This function adds a straight line segment from the current point to the
 * specified coordinates. This is equivalent to the `L` command in SVG path syntax.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param x The x-coordinate of the end point of the line segment.
 * @param y The y-coordinate of the end point of the line segment.
 */
PLUTOVG_API bool plutovg_path_line_to(plutovg_path_t* path, float x, float y);

/**
 * @brief Adds a quadratic Bézier curve to the path.
 *
 * This function adds a quadratic Bézier curve segment from the current point
 * to the specified end point, using the given control point. This is equivalent
 * to the `Q` command in SVG path syntax.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param x1 The x-coordinate of the control point.
 * @param y1 The y-coordinate of the control point.
 * @param x2 The x-coordinate of the end point of the curve.
 * @param y2 The y-coordinate of the end point of the curve.
 */
PLUTOVG_API bool plutovg_path_quad_to(plutovg_path_t* path, float x1, float y1, float x2, float y2);

/**
 * @brief Adds a cubic Bézier curve to the path.
 *
 * This function adds a cubic Bézier curve segment from the current point
 * to the specified end point, using the given two control points. This is
 * equivalent to the `C` command in SVG path syntax.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param x1 The x-coordinate of the first control point.
 * @param y1 The y-coordinate of the first control point.
 * @param x2 The x-coordinate of the second control point.
 * @param y2 The y-coordinate of the second control point.
 * @param x3 The x-coordinate of the end point of the curve.
 * @param y3 The y-coordinate of the end point of the curve.
 */
PLUTOVG_API bool plutovg_path_cubic_to(plutovg_path_t* path, float x1, float y1, float x2, float y2, float x3, float y3);

/**
 * @brief Adds an elliptical arc to the path.
 *
 * This function adds an elliptical arc segment from the current point to the
 * specified end point. The arc is defined by the radii, rotation angle, and
 * flags for large arc and sweep. This is equivalent to the `A` command in SVG
 * path syntax.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param rx The x-radius of the ellipse.
 * @param ry The y-radius of the ellipse.
 * @param angle The rotation angle of the ellipse in radians.
 * @param large_arc_flag If true, draw the large arc; otherwise, draw the small arc.
 * @param sweep_flag If true, draw the arc in the positive-angle direction; otherwise, in the negative-angle direction.
 * @param x The x-coordinate of the end point of the arc.
 * @param y The y-coordinate of the end point of the arc.
 */
PLUTOVG_API bool plutovg_path_arc_to(plutovg_path_t* path, float rx, float ry, float angle, bool large_arc_flag, bool sweep_flag, float x, float y);

/**
 * @brief Closes the current sub-path.
 *
 * This function closes the current sub-path by drawing a straight line back to
 * the start point of the sub-path. This is equivalent to the `Z` command in SVG
 * path syntax.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 */
PLUTOVG_API bool plutovg_path_close(plutovg_path_t* path);

/**
 * @brief Retrieves the current point of the path.
 *
 * Gets the current point's coordinates in the path. This point is the last
 * position used or the point where the path was last moved to.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param x The x-coordinate of the current point.
 * @param y The y-coordinate of the current point.
 */
PLUTOVG_API void plutovg_path_get_current_point(const plutovg_path_t* path, float* x, float* y);

/**
 * @brief Reserves space for path elements.
 *
 * Reserves space for a specified number of elements in the path. This helps optimize
 * memory allocation for future path operations.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param count The number of path elements to reserve space for.
 */
PLUTOVG_API bool plutovg_path_reserve(plutovg_path_t* path, int count);

/**
 * @brief Resets the path.
 *
 * Clears all path data, effectively resetting the `plutovg_path_t` object to its initial state.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 */
PLUTOVG_API void plutovg_path_reset(plutovg_path_t* path);

/**
 * @brief Adds a rectangle to the path.
 *
 * Adds a rectangle defined by the top-left corner (x, y) and dimensions (w, h) to the path.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param x The x-coordinate of the rectangle's top-left corner.
 * @param y The y-coordinate of the rectangle's top-left corner.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
PLUTOVG_API bool plutovg_path_add_rect(plutovg_path_t* path, float x, float y, float w, float h);

/**
 * @brief Adds a rounded rectangle to the path.
 *
 * Adds a rounded rectangle defined by the top-left corner (x, y), dimensions (w, h),
 * and corner radii (rx, ry) to the path.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param x The x-coordinate of the rectangle's top-left corner.
 * @param y The y-coordinate of the rectangle's top-left corner.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @param rx The x-radius of the rectangle's corners.
 * @param ry The y-radius of the rectangle's corners.
 */
PLUTOVG_API bool plutovg_path_add_round_rect(plutovg_path_t* path, float x, float y, float w, float h, float rx, float ry);

/**
 * @brief Adds an ellipse to the path.
 *
 * Adds an ellipse defined by the center (cx, cy) and radii (rx, ry) to the path.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param cx The x-coordinate of the ellipse's center.
 * @param cy The y-coordinate of the ellipse's center.
 * @param rx The x-radius of the ellipse.
 * @param ry The y-radius of the ellipse.
 */
PLUTOVG_API bool plutovg_path_add_ellipse(plutovg_path_t* path, float cx, float cy, float rx, float ry);

/**
 * @brief Adds a circle to the path.
 *
 * Adds a circle defined by its center (cx, cy) and radius (r) to the path.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param cx The x-coordinate of the circle's center.
 * @param cy The y-coordinate of the circle's center.
 * @param r The radius of the circle.
 */
PLUTOVG_API bool plutovg_path_add_circle(plutovg_path_t* path, float cx, float cy, float r);

/**
 * @brief Adds an arc to the path.
 *
 * Adds an arc defined by the center (cx, cy), radius (r), start angle (a0), end angle (a1),
 * and direction (ccw) to the path.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param cx The x-coordinate of the arc's center.
 * @param cy The y-coordinate of the arc's center.
 * @param r The radius of the arc.
 * @param a0 The start angle of the arc in radians.
 * @param a1 The end angle of the arc in radians.
 * @param ccw If true, the arc is drawn counter-clockwise; if false, clockwise.
 */
PLUTOVG_API bool plutovg_path_add_arc(plutovg_path_t* path, float cx, float cy, float r, float a0, float a1, bool ccw);

/**
 * @brief Adds a sub-path to the path.
 *
 * Adds all elements from another path (`source`) to the current path, optionally
 * applying a transformation matrix.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param source A pointer to the `plutovg_path_t` object to copy elements from.
 * @param matrix A pointer to a `::gfx::matrix` object, or `NULL` to apply no transformation.
 */
PLUTOVG_API bool plutovg_path_add_path(plutovg_path_t* path, const plutovg_path_t* source, const ::gfx::matrix* matrix);

/**
 * @brief Applies a transformation matrix to the path.
 *
 * Transforms the entire path using the provided transformation matrix.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param matrix A pointer to a `::gfx::matrix` object.
 */
PLUTOVG_API void plutovg_path_transform(plutovg_path_t* path, const ::gfx::matrix* matrix);

/**
 * @brief Callback function type for traversing a path.
 *
 * This function type defines a callback used to traverse path elements.
 *
 * @param closure A pointer to user-defined data passed to the callback.
 * @param command The current path command.
 * @param points An array of points associated with the command.
 * @param npoints The number of points in the array.
 */
typedef bool (*plutovg_path_traverse_func_t)(void* closure, plutovg_path_command_t command, const ::gfx::pointf* points, int npoints);

/**
 * @brief Traverses the path and calls the callback for each element.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param traverse_func The callback function to be called for each element of the path.
 * @param closure User-defined data passed to the callback.
 */
PLUTOVG_API bool plutovg_path_traverse(const plutovg_path_t* path, plutovg_path_traverse_func_t traverse_func, void* closure);

/**
 * @brief Traverses the path with Bézier curves flattened to line segments.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param traverse_func The callback function to be called for each element of the path.
 * @param closure User-defined data passed to the callback.
 */
PLUTOVG_API bool plutovg_path_traverse_flatten(const plutovg_path_t* path, plutovg_path_traverse_func_t traverse_func, void* closure);

/**
 * @brief Traverses the path with a dashed pattern and calls the callback for each segment.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param offset The starting offset into the dash pattern.
 * @param dashes An array of dash lengths.
 * @param ndashes The number of elements in the `dashes` array.
 * @param traverse_func The callback function to be called for each element of the path.
 * @param closure User-defined data passed to the callback.
 */
PLUTOVG_API bool plutovg_path_traverse_dashed(const plutovg_path_t* path, float offset, const float* dashes, int ndashes, plutovg_path_traverse_func_t traverse_func, void* closure);

/**
 * @brief Creates a copy of the path.
 *
 * @param path A pointer to the `plutovg_path_t` object to clone.
 * @return A pointer to the newly created path clone.
 */
PLUTOVG_API plutovg_path_t* plutovg_path_clone(const plutovg_path_t* path,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief Creates a copy of the path with Bézier curves flattened to line segments.
 *
 * @param path A pointer to the `plutovg_path_t` object to clone.
 * @return A pointer to the newly created path clone with flattened curves.
 */
PLUTOVG_API plutovg_path_t* plutovg_path_clone_flatten(const plutovg_path_t* path,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief Creates a copy of the path with a dashed pattern applied.
 *
 * @param path A pointer to the `plutovg_path_t` object to clone.
 * @param offset The starting offset into the dash pattern.
 * @param dashes An array of dash lengths.
 * @param ndashes The number of elements in the `dashes` array.
 * @return A pointer to the newly created path clone with dashed pattern.
 */
PLUTOVG_API plutovg_path_t* plutovg_path_clone_dashed(const plutovg_path_t* path, float offset, const float* dashes, int ndashes,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief Computes the bounding box and total length of the path.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @param extents A pointer to a `plutovg_rect_t` object to store the bounding box.
 * @param tight If `true`, computes a precise bounding box; otherwise, aligns to control points.
 * @return The total length of the path.
 */
PLUTOVG_API float plutovg_path_extents(const plutovg_path_t* path, plutovg_rect_t* extents, bool tight);

/**
 * @brief Calculates the total length of the path.
 *
 * @param path A pointer to a `plutovg_path_t` object.
 * @return The total length of the path.
 */
PLUTOVG_API float plutovg_path_length(const plutovg_path_t* path);

/**
 * @brief Parses SVG path data into a `plutovg_path_t` object.
 *
 * @param path A pointer to the `plutovg_path_t` object to populate.
 * @param data The SVG path data string.
 * @param length The length of `data`, or `-1` for null-terminated data.
 * @return `true` if successful; `false` otherwise.
 */
PLUTOVG_API bool plutovg_path_parse(plutovg_path_t* path, const char* data, int length);

/**
 * @brief Text encodings used for converting text data to code points.
 */
typedef enum plutovg_text_encoding {
    PLUTOVG_TEXT_ENCODING_UTF8, ///< UTF-8 encoding
    PLUTOVG_TEXT_ENCODING_UTF16, ///< UTF-16 encoding
    PLUTOVG_TEXT_ENCODING_UTF32, ///< UTF-32 encoding
    PLUTOVG_TEXT_ENCODING_LATIN1 ///< Latin-1 encoding
} plutovg_text_encoding_t;

/**
 * @brief Iterator for traversing code points in text data.
 */
typedef struct plutovg_text_iterator {
    const void* text; ///< Pointer to the text data.
    int length; ///< Length of the text data.
    plutovg_text_encoding_t encoding; ///< Encoding format of the text data.
    int index; ///< Current position in the text data.
} plutovg_text_iterator_t;

/**
 * @brief Represents a Unicode code point.
 */
typedef unsigned int plutovg_codepoint_t;

/**
 * @brief Initializes a text iterator.
 *
 * @param it Pointer to the text iterator.
 * @param text Pointer to the text data.
 * @param length Length of the text data, or -1 if the data is null-terminated.
 * @param encoding Encoding of the text data.
 */
PLUTOVG_API void plutovg_text_iterator_init(plutovg_text_iterator_t* it, const void* text, int length, plutovg_text_encoding_t encoding);

/**
 * @brief Checks if there are more code points to iterate.
 *
 * @param it Pointer to the text iterator.
 * @return `true` if more code points are available; otherwise, `false`.
 */
PLUTOVG_API bool plutovg_text_iterator_has_next(const plutovg_text_iterator_t* it);

/**
 * @brief Retrieves the next code point and advances the iterator.
 *
 * @param it Pointer to the text iterator.
 * @return The next code point.
 */
PLUTOVG_API plutovg_codepoint_t plutovg_text_iterator_next(plutovg_text_iterator_t* it);

/**
 * @brief Represents a font face.
 */
typedef struct plutovg_font_face plutovg_font_face_t;

/**
 * @brief Loads a font face from memory.
 *
 * @param data Pointer to the font data.
 * @param ttcindex Index of the font face within a TrueType Collection (TTC).
 * @param destroy_func Function to free the font data when no longer needed.
 * @param closure User-defined data passed to `destroy_func`.
 * @return A pointer to the loaded `plutovg_font_face_t` object, or `NULL` on failure.
 */
PLUTOVG_API plutovg_font_face_t* plutovg_font_face_load_from_stream(gfx::stream& data, int ttcindex, plutovg_destroy_func_t destroy_func, void* closure);

/**
 * @brief Increments the reference count of a font face.
 *
 * @param face A pointer to a `plutovg_font_face_t` object.
 * @return A pointer to the same `plutovg_font_face_t` object with an incremented reference count.
 */
PLUTOVG_API plutovg_font_face_t* plutovg_font_face_reference(plutovg_font_face_t* face);

/**
 * @brief Decrements the reference count and potentially destroys the font face.
 *
 * @param face A pointer to a `plutovg_font_face_t` object.
 */
PLUTOVG_API void plutovg_font_face_destroy(plutovg_font_face_t* face);

/**
 * @brief Retrieves the current reference count of a font face.
 *
 * @param face A pointer to a `plutovg_font_face_t` object.
 * @return The reference count of the font face.
 */
PLUTOVG_API int plutovg_font_face_get_reference_count(const plutovg_font_face_t* face);

/**
 * @brief Retrieves metrics for a font face at a specified size.
 *
 * @param face A pointer to a `plutovg_font_face_t` object.
 * @param size The font size in pixels.
 * @param ascent Pointer to store the ascent metric.
 * @param descent Pointer to store the descent metric.
 * @param line_gap Pointer to store the line gap metric.
 * @param extents Pointer to a `plutovg_rect_t` object to store the font bounding box.
 */
PLUTOVG_API void plutovg_font_face_get_metrics(const plutovg_font_face_t* face, float size, float* ascent, float* descent, float* line_gap, plutovg_rect_t* extents);

/**
 * @brief Retrieves metrics for a specified glyph at a given size.
 *
 * @param face A pointer to a `plutovg_font_face_t` object.
 * @param size The font size in pixels.
 * @param codepoint The Unicode code point of the glyph.
 * @param advance_width Pointer to store the advance width of the glyph.
 * @param left_side_bearing Pointer to store the left side bearing of the glyph.
 * @param extents Pointer to a `plutovg_rect_t` object to store the glyph bounding box.
 */
PLUTOVG_API void plutovg_font_face_get_glyph_metrics(const plutovg_font_face_t* face, float size, plutovg_codepoint_t codepoint, float* advance_width, float* left_side_bearing, plutovg_rect_t* extents);

/**
 * @brief Retrieves the path of a glyph and its advance width.
 *
 * @param face A pointer to a `plutovg_font_face_t` object.
 * @param size The font size in pixels.
 * @param x The x-coordinate for positioning the glyph.
 * @param y The y-coordinate for positioning the glyph.
 * @param codepoint The Unicode code point of the glyph.
 * @param path Pointer to a `plutovg_path_t` object to store the glyph path.
 * @return The advance width of the glyph.
 */
PLUTOVG_API float plutovg_font_face_get_glyph_path(const plutovg_font_face_t* face, float size, float x, float y, plutovg_codepoint_t codepoint, plutovg_path_t* path);

/**
 * @brief Traverses the path of a glyph and calls a callback for each path element.
 *
 * @param face A pointer to a `plutovg_font_face_t` object.
 * @param size The font size in pixels.
 * @param x The x-coordinate for positioning the glyph.
 * @param y The y-coordinate for positioning the glyph.
 * @param codepoint The Unicode code point of the glyph.
 * @param traverse_func The callback function to be called for each path element.
 * @param closure User-defined data passed to the callback function.
 * @return The advance width of the glyph.
 */
PLUTOVG_API float plutovg_font_face_traverse_glyph_path(const plutovg_font_face_t* face, float size, float x, float y, plutovg_codepoint_t codepoint, plutovg_path_traverse_func_t traverse_func, void* closure);

/**
 * @brief Computes the bounding box of a text string and its advance width.
 *
 * @param face A pointer to a `plutovg_font_face_t` object.
 * @param size The font size in pixels.
 * @param text Pointer to the text data.
 * @param length Length of the text data, or -1 if null-terminated.
 * @param encoding Encoding of the text data.
 * @param extents Pointer to a `plutovg_rect_t` object to store the bounding box of the text.
 * @return The total advance width of the text.
 */
PLUTOVG_API float plutovg_font_face_text_extents(const plutovg_font_face_t* face, float size, const void* text, int length, plutovg_text_encoding_t encoding, plutovg_rect_t* extents);

/**
 * @brief Represents a color with red, green, blue, and alpha components.
 */
typedef struct plutovg_color {
    float r; ///< Red component (0 to 1).
    float g; ///< Green component (0 to 1).
    float b; ///< Blue component (0 to 1).
    float a; ///< Alpha (opacity) component (0 to 1).
} plutovg_color_t;

#define PLUTOVG_MAKE_COLOR(r, g, b, a) ((plutovg_color_t){r, g, b, a})

#define PLUTOVG_BLACK_COLOR   PLUTOVG_MAKE_COLOR(0, 0, 0, 1)
#define PLUTOVG_WHITE_COLOR   PLUTOVG_MAKE_COLOR(1, 1, 1, 1)
#define PLUTOVG_RED_COLOR     PLUTOVG_MAKE_COLOR(1, 0, 0, 1)
#define PLUTOVG_GREEN_COLOR   PLUTOVG_MAKE_COLOR(0, 1, 0, 1)
#define PLUTOVG_BLUE_COLOR    PLUTOVG_MAKE_COLOR(0, 0, 1, 1)
#define PLUTOVG_YELLOW_COLOR  PLUTOVG_MAKE_COLOR(1, 1, 0, 1)
#define PLUTOVG_CYAN_COLOR    PLUTOVG_MAKE_COLOR(0, 1, 1, 1)
#define PLUTOVG_MAGENTA_COLOR PLUTOVG_MAKE_COLOR(1, 0, 1, 1)

PLUTOVG_API void plutovg_color_init_rgb(plutovg_color_t* color, float r, float g, float b);
PLUTOVG_API void plutovg_color_init_rgba(plutovg_color_t* color, float r, float g, float b, float a);
PLUTOVG_API void plutovg_color_init_col(plutovg_color_t* color, ::gfx::vector_pixel px);
PLUTOVG_API void plutovg_color_init_rgb8(plutovg_color_t* color, int r, int g, int b);
PLUTOVG_API void plutovg_color_init_rgba8(plutovg_color_t* color, int r, int g, int b, int a);
PLUTOVG_API void plutovg_color_init_bgra32(plutovg_color_t* color, unsigned int value);
PLUTOVG_API void plutovg_color_init_rgba32(plutovg_color_t* color, unsigned int value);
PLUTOVG_API void plutovg_color_init_argb32(plutovg_color_t* color, unsigned int value);

PLUTOVG_API unsigned int plutovg_color_to_rgba32(const plutovg_color_t* color);
PLUTOVG_API unsigned int plutovg_color_to_argb32(const plutovg_color_t* color);

PLUTOVG_API int plutovg_color_parse(plutovg_color_t* color, const char* data, int length);

/**
 * @brief Represents an image surface for drawing operations.
 *
 * The pixel data is stored in a premultiplied 32-bit ARGB format (0xAARRGGBB).
 * The red, green, and blue channels are multiplied by the alpha component divided by 255.
 * Premultiplied ARGB32 is beneficial for faster operations such as alpha blending.
 */
typedef struct plutovg_surface plutovg_surface_t;

/**
 * @brief Creates a new image surface with the specified dimensions.
 *
 * @param width The width of the surface in pixels.
 * @param height The height of the surface in pixels.
 * @return A pointer to the newly created `plutovg_surface_t` object.
 */
PLUTOVG_API plutovg_surface_t* plutovg_surface_create(int width, int height);

/**
 * @brief Creates an image surface using existing pixel data.
 *
 * @param data Pointer to the pixel data.
 * @param width The width of the surface in pixels.
 * @param height The height of the surface in pixels.
 * @param stride The number of bytes per row in the pixel data.
 * @return A pointer to the newly created `plutovg_surface_t` object.
 */
PLUTOVG_API plutovg_surface_t* plutovg_surface_create_for_data(unsigned char* data, int width, int height, int stride);

/**
 * @brief Loads an image surface from a file.
 *
 * @param filename Path to the image file.
 * @return Pointer to the surface, or `NULL` on failure.
 */
PLUTOVG_API plutovg_surface_t* plutovg_surface_load_from_image_file(const char* filename);

/**
 * @brief Loads an image surface from raw image data.
 *
 * @param data Pointer to the image data.
 * @param length Length of the data in bytes.
 * @return Pointer to the surface, or `NULL` on failure.
 */
PLUTOVG_API plutovg_surface_t* plutovg_surface_load_from_image_data(const void* data, int length);

PLUTOVG_API plutovg_surface_t* plutovg_surface_load_from_image(unsigned char* image, int width, int height);

/**
 * @brief Loads an image surface from base64-encoded data.
 *
 * @param data Pointer to the base64-encoded image data.
 * @param length Length of the data in bytes, or `-1` if null-terminated.
 * @return Pointer to the surface, or `NULL` on failure.
 */
PLUTOVG_API plutovg_surface_t* plutovg_surface_load_from_image_base64(const char* data, int length);

/**
 * @brief Increments the reference count for a surface.
 *
 * @param surface Pointer to the `plutovg_surface_t` object.
 * @return Pointer to the `plutovg_surface_t` object.
 */
PLUTOVG_API plutovg_surface_t* plutovg_surface_reference(plutovg_surface_t* surface);

/**
 * @brief Decrements the reference count and destroys the surface if the count reaches zero.
 *
 * @param surface Pointer to the `plutovg_surface_t` object .
 */
PLUTOVG_API void plutovg_surface_destroy(plutovg_surface_t* surface);

/**
 * @brief Gets the current reference count of a surface.
 *
 * @param surface Pointer to the `plutovg_surface_t` object.
 * @return The reference count of the surface.
 */
PLUTOVG_API int plutovg_surface_get_reference_count(const plutovg_surface_t* surface);

/**
 * @brief Gets the pixel data of the surface.
 *
 * @param surface Pointer to the `plutovg_surface_t` object.
 * @return Pointer to the pixel data.
 */
PLUTOVG_API unsigned char* plutovg_surface_get_data(const plutovg_surface_t* surface);

/**
 * @brief Gets the width of the surface.
 *
 * @param surface Pointer to the `plutovg_surface_t` object.
 * @return Width of the surface in pixels.
 */
PLUTOVG_API int plutovg_surface_get_width(const plutovg_surface_t* surface);

/**
 * @brief Gets the height of the surface.
 *
 * @param surface Pointer to the `plutovg_surface_t` object.
 * @return Height of the surface in pixels.
 */
PLUTOVG_API int plutovg_surface_get_height(const plutovg_surface_t* surface);

/**
 * @brief Gets the stride of the surface.
 *
 * @param surface Pointer to the `plutovg_surface_t` object.
 * @return Number of bytes per row.
 */
PLUTOVG_API int plutovg_surface_get_stride(const plutovg_surface_t* surface);

/**
 * @brief plutovg_surface_clear
 * @param surface
 * @param color
 */
PLUTOVG_API void plutovg_surface_clear(plutovg_surface_t* surface, const plutovg_color_t* color);

/**
 * @brief Writes the surface to a PNG file.
 *
 * @param surface Pointer to the `plutovg_surface_t` object.
 * @param filename Path to the output PNG file.
 * @return `true` if successful, `false` otherwise.
 */
PLUTOVG_API bool plutovg_surface_write_to_png(const plutovg_surface_t* surface, const char* filename);

/**
 * @brief Writes the surface to a JPEG file.
 *
 * @param surface Pointer to the `plutovg_surface_t` object.
 * @param filename Path to the output JPEG file.
 * @param quality JPEG quality (0 to 100).
 * @return `true` if successful, `false` otherwise.
 */
PLUTOVG_API bool plutovg_surface_write_to_jpg(const plutovg_surface_t* surface, const char* filename, int quality);

/**
 * @brief Writes the surface to a PNG stream.
 *
 * @param surface Pointer to the `plutovg_surface_t` object.
 * @param write_func Callback function for writing data.
 * @param closure User-defined data passed to the callback.
 * @return `true` if successful, `false` otherwise.
 */
PLUTOVG_API bool plutovg_surface_write_to_png_stream(const plutovg_surface_t* surface, plutovg_write_func_t write_func, void* closure);

/**
 * @brief Writes the surface to a JPEG stream.
 *
 * @param surface Pointer to the `plutovg_surface_t` object.
 * @param write_func Callback function for writing data.
 * @param closure User-defined data passed to the callback.
 * @param quality JPEG quality (0 to 100).
 * @return `true` if successful, `false` otherwise.
 */
PLUTOVG_API bool plutovg_surface_write_to_jpg_stream(const plutovg_surface_t* surface, plutovg_write_func_t write_func, void* closure, int quality);

/**
 * @brief Converts ARGB Premultiplied to RGBA Plain.
 *
 * @param dst Destination buffer (can be the same as `src`).
 * @param src Source buffer (ARGB Premultiplied).
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param stride Image stride in bytes.
 */
PLUTOVG_API void plutovg_convert_rgba_to_abgr(unsigned char* dst, const unsigned char* src, int width, int height, int stride);

/**
 * @brief Converts RGBA Plain to ARGB Premultiplied.
 *
 * @param dst Destination buffer (can be the same as `src`).
 * @param src Source buffer (RGBA Plain).
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param stride Image stride in bytes.
 */
PLUTOVG_API void plutovg_convert_abgr_to_rgba(unsigned char* dst, const unsigned char* src, int width, int height, int stride);

/**
 * @brief Defines the type of texture, either plain or tiled.
 */
typedef enum {
    PLUTOVG_TEXTURE_TYPE_PLAIN, ///< Plain texture.
    PLUTOVG_TEXTURE_TYPE_TILED ///< Tiled texture.
} plutovg_texture_type_t;

/**
 * @brief Defines the spread method for gradients.
 */
typedef enum {
    PLUTOVG_SPREAD_METHOD_PAD, ///< Pad the gradient's edges.
    PLUTOVG_SPREAD_METHOD_REFLECT, ///< Reflect the gradient beyond its bounds.
    PLUTOVG_SPREAD_METHOD_REPEAT ///< Repeat the gradient pattern.
} plutovg_spread_method_t;

/**
 * @brief Represents a gradient stop.
 */

/*typedef struct {
    float offset; ///< The offset of the gradient stop, as a value between 0 and 1.
    plutovg_color_t color; ///< The color of the gradient stop.
}*/
typedef ::gfx::gradient_stop plutovg_gradient_stop_t;

/**
 * @brief Represents a paint object used for drawing operations.
 */
typedef struct plutovg_paint plutovg_paint_t;

/**
 * @brief Creates a solid RGB paint.
 *
 * @param r The red component (0 to 1).
 * @param g The green component (0 to 1).
 * @param b The blue component (0 to 1).
 * @return A pointer to the created `plutovg_paint_t` object.
 */
PLUTOVG_API plutovg_paint_t* plutovg_paint_create_rgb(float r, float g, float b,void*(*allocator)(size_t));

/**
 * @brief Creates a solid RGBA paint.
 *
 * @param r The red component (0 to 1).
 * @param g The green component (0 to 1).
 * @param b The blue component (0 to 1).
 * @param a The alpha component (0 to 1).
 * @return A pointer to the created `plutovg_paint_t` object.
 */
PLUTOVG_API plutovg_paint_t* plutovg_paint_create_rgba(float r, float g, float b, float a,void*(*allocator)(size_t));

/**
 * @brief Creates a solid color paint.
 *
 * @param color A pointer to the `plutovg_color_t` object.
 * @return A pointer to the created `plutovg_paint_t` object.
 */
PLUTOVG_API plutovg_paint_t* plutovg_paint_create_color(const plutovg_color_t* color,void*(*allocator)(size_t));

/**
 * @brief Creates a linear gradient paint.
 *
 * @param x1 The x coordinate of the gradient start.
 * @param y1 The y coordinate of the gradient start.
 * @param x2 The x coordinate of the gradient end.
 * @param y2 The y coordinate of the gradient end.
 * @param spread The gradient spread method.
 * @param stops Array of gradient stops.
 * @param nstops Number of gradient stops.
 * @param matrix Optional transformation matrix.
 * @return A pointer to the created `plutovg_paint_t` object.
 */
PLUTOVG_API plutovg_paint_t* plutovg_paint_create_linear_gradient(float x1, float y1, float x2, float y2,
    plutovg_spread_method_t spread, const plutovg_gradient_stop_t* stops, int nstops, const ::gfx::matrix* matrix,void*(*allocator)(size_t));

/**
 * @brief Creates a radial gradient paint.
 *
 * @param cx The x coordinate of the gradient center.
 * @param cy The y coordinate of the gradient center.
 * @param cr The radius of the gradient.
 * @param fx The x coordinate of the focal point.
 * @param fy The y coordinate of the focal point.
 * @param fr The radius of the focal point.
 * @param spread The gradient spread method.
 * @param stops Array of gradient stops.
 * @param nstops Number of gradient stops.
 * @param matrix Optional transformation matrix.
 * @return A pointer to the created `plutovg_paint_t` object.
 */
PLUTOVG_API plutovg_paint_t* plutovg_paint_create_radial_gradient(float cx, float cy, float cr, float fx, float fy, float fr,
    plutovg_spread_method_t spread, const plutovg_gradient_stop_t* stops, int nstops, const ::gfx::matrix* matrix,void*(*allocator)(size_t));

/**
 * @brief Creates a texture paint from a surface.
 *
 * @param surface The texture surface.
 * @param type The texture type (plain or tiled).
 * @param opacity The opacity of the texture (0 to 1).
 * @param matrix Optional transformation matrix.
 * @return A pointer to the created `plutovg_paint_t` object.
 */
PLUTOVG_API plutovg_paint_t* plutovg_paint_create_texture(int width, int height, const ::gfx::vector_on_read_callback_type callback, void* callback_state, plutovg_texture_type_t type, float opacity, const ::gfx::matrix* matrix,void*(*allocator)(size_t));

PLUTOVG_API plutovg_paint_t* plutovg_paint_create_texture_direct(int width, int height, const::gfx::const_blt_span* direct,  const ::gfx::on_direct_read_callback_type callback, plutovg_texture_type_t type, float opacity, const ::gfx::matrix* matrix,void*(*allocator)(size_t));

/**
 * @brief Increments the reference count of a paint object.
 *
 * @param paint A pointer to the `plutovg_paint_t` object.
 * @return A pointer to the referenced `plutovg_paint_t` object.
 */
PLUTOVG_API plutovg_paint_t* plutovg_paint_reference(plutovg_paint_t* paint);

/**
 * @brief Decrements the reference count and destroys the paint if the count reaches zero.
 *
 * @param paint A pointer to the `plutovg_paint_t` object.
 */
PLUTOVG_API void plutovg_paint_destroy(plutovg_paint_t* paint,void(*deallocator)(void*));

/**
 * @brief Retrieves the reference count of a paint object.
 *
 * @param paint A pointer to the `plutovg_paint_t` object.
 * @return The reference count of the `plutovg_paint_t` object.
 */
PLUTOVG_API int plutovg_paint_get_reference_count(const plutovg_paint_t* paint);

/**
 * @brief Defines fill rule types for filling paths.
 */
typedef enum {
    PLUTOVG_FILL_RULE_NON_ZERO, ///< Non-zero winding fill rule.
    PLUTOVG_FILL_RULE_EVEN_ODD ///< Even-odd fill rule.
} plutovg_fill_rule_t;

/**
 * @brief Defines compositing operations.
 */
typedef enum {
    PLUTOVG_OPERATOR_SRC, ///< Source replaces destination.
    PLUTOVG_OPERATOR_SRC_OVER, ///< Source over destination.
    PLUTOVG_OPERATOR_DST_IN, ///< Destination within source.
    PLUTOVG_OPERATOR_DST_OUT, ///< Destination outside source.
    PLUTOVG_LAST
} plutovg_operator_t;

/**
 * @brief Defines the shape used at the ends of open subpaths.
 */
typedef enum {
    PLUTOVG_LINE_CAP_BUTT, ///< Flat edge at the end of the stroke.
    PLUTOVG_LINE_CAP_ROUND, ///< Rounded ends at the end of the stroke.
    PLUTOVG_LINE_CAP_SQUARE ///< Square ends at the end of the stroke.
} plutovg_line_cap_t;

/**
 * @brief Defines the shape used at the corners of paths.
 */
typedef enum {
    PLUTOVG_LINE_JOIN_MITER, ///< Miter join with sharp corners.
    PLUTOVG_LINE_JOIN_ROUND, ///< Rounded join.
    PLUTOVG_LINE_JOIN_BEVEL ///< Beveled join with a flattened corner.
} plutovg_line_join_t;

/**
 * @brief Represents a drawing context.
 */
typedef struct plutovg_canvas plutovg_canvas_t;
/**
 * @brief Creates a drawing context on a surface.
 *
 * @param surface A pointer to a `plutovg_surface_t` object.
 * @return A pointer to the newly created `plutovg_canvas_t` object.
 */
PLUTOVG_API plutovg_canvas* plutovg_canvas_create(int width, int height, void*(*allocator)(size_t), void*(*reallocator)(void*,size_t), void(*deallocator)(void*));

PLUTOVG_API void plutovg_canvas_set_callbacks(plutovg_canvas_t* canvas,plutovg_write_callback_t write_callback,plutovg_read_callback_t read_callback, void* callback_state);
PLUTOVG_API ::gfx::blt_span* plutovg_canvas_get_direct(plutovg_canvas_t* canvas);
PLUTOVG_API int plutovg_canvas_get_direct_width(plutovg_canvas_t* canvas);
PLUTOVG_API int plutovg_canvas_get_direct_height(plutovg_canvas_t* canvas);
PLUTOVG_API int plutovg_canvas_get_direct_offset_x(plutovg_canvas_t* canvas);
PLUTOVG_API int plutovg_canvas_get_direct_offset_y(plutovg_canvas_t* canvas);
PLUTOVG_API ::gfx::on_direct_read_callback_type plutovg_canvas_get_direct_on_read(plutovg_canvas_t* canvas);
PLUTOVG_API ::gfx::on_direct_write_callback_type plutovg_canvas_get_direct_on_write(plutovg_canvas_t* canvas);
PLUTOVG_API void plutovg_canvas_global_clip(plutovg_canvas_t* canvas, const plutovg_rect_t* rect);
PLUTOVG_API void plutovg_canvas_set_direct(plutovg_canvas_t* canvas, ::gfx::blt_span* direct, int offset_x, int offset_y, int width, int height,::gfx::on_direct_read_callback_type on_read,::gfx::on_direct_write_callback_type on_write);
/**
 * @brief Increases the reference count of the canvas.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @return The same pointer to the `plutovg_canvas_t` object.
 */
PLUTOVG_API plutovg_canvas_t* plutovg_canvas_reference(plutovg_canvas_t* canvas);

/**
 * @brief Decreases the reference count and destroys the canvas when it reaches zero.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 */
PLUTOVG_API void plutovg_canvas_destroy(plutovg_canvas_t* canvas);

/**
 * @brief Retrieves the reference count of the canvas.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @return The current reference count.
 */
PLUTOVG_API int plutovg_canvas_get_reference_count(const plutovg_canvas_t* canvas);

/**
 * @brief Gets the surface associated with the canvas.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @return A pointer to the `plutovg_surface_t` object.
 */
PLUTOVG_API plutovg_surface_t* plutovg_canvas_get_surface(const plutovg_canvas_t* canvas);

/**
 * @brief Saves the current state of the canvas.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 */
PLUTOVG_API bool plutovg_canvas_save(plutovg_canvas_t* canvas);

/**
 * @brief Restores the canvas to the most recently saved state.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 */
PLUTOVG_API void plutovg_canvas_restore(plutovg_canvas_t* canvas);

/**
 * @brief Sets the current paint to a solid color.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param r The red component (0 to 1).
 * @param g The green component (0 to 1).
 * @param b The blue component (0 to 1).
 */
PLUTOVG_API bool plutovg_canvas_set_rgb(plutovg_canvas_t* canvas, float r, float g, float b);

/**
 * @brief Sets the current paint to a solid color.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param r The red component (0 to 1).
 * @param g The green component (0 to 1).
 * @param b The blue component (0 to 1).
 * @param a The alpha component (0 to 1).
 */
PLUTOVG_API bool plutovg_canvas_set_rgba(plutovg_canvas_t* canvas, float r, float g, float b, float a);

/**
 * @brief Sets the current paint to a solid color.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param color A pointer to a `plutovg_color_t` object.
 */
PLUTOVG_API bool plutovg_canvas_set_color(plutovg_canvas_t* canvas, const plutovg_color_t* color);

/**
 * @brief Sets the current paint to a linear gradient.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param x1 The x coordinate of the start point.
 * @param y1 The y coordinate of the start point.
 * @param x2 The x coordinate of the end point.
 * @param y2 The y coordinate of the end point.
 * @param spread The gradient spread method.
 * @param stops Array of gradient stops.
 * @param nstops Number of gradient stops.
 * @param matrix Optional transformation matrix.
 */
PLUTOVG_API bool plutovg_canvas_set_linear_gradient(plutovg_canvas_t* canvas, float x1, float y1, float x2, float y2,
    plutovg_spread_method_t spread, const plutovg_gradient_stop_t* stops, int nstops, const ::gfx::matrix* matrix);

/**
 * @brief Sets the current paint to a radial gradient.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param cx The x coordinate of the center.
 * @param cy The y coordinate of the center.
 * @param cr The radius of the gradient.
 * @param fx The x coordinate of the focal point.
 * @param fy The y coordinate of the focal point.
 * @param fr The radius of the focal point.
 * @param spread The gradient spread method.
 * @param stops Array of gradient stops.
 * @param nstops Number of gradient stops.
 * @param matrix Optional transformation matrix.
 */
PLUTOVG_API bool plutovg_canvas_set_radial_gradient(plutovg_canvas_t* canvas, float cx, float cy, float cr, float fx, float fy, float fr,
    plutovg_spread_method_t spread, const plutovg_gradient_stop_t* stops, int nstops, const ::gfx::matrix* matrix);

/**
 * @brief Sets the current paint to a texture.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param surface The texture surface.
 * @param type The texture type (plain or tiled).
 * @param opacity The opacity of the texture (0 to 1).
 * @param matrix Optional transformation matrix.
 */
PLUTOVG_API bool plutovg_canvas_set_texture(plutovg_canvas_t* canvas, int width, int height,::gfx::vector_on_read_callback_type callback, void* callback_state, plutovg_texture_type_t type, float opacity, const ::gfx::matrix* matrix);
PLUTOVG_API bool plutovg_canvas_set_texture_direct(plutovg_canvas_t* canvas, int width, int height,const ::gfx::const_blt_span* direct, ::gfx::on_direct_read_callback_type direct_callback, plutovg_texture_type_t type, float opacity, const ::gfx::matrix* matrix);

/**
 * @brief Sets the current paint.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param paint The paint to be used for subsequent drawing operations.
 */
PLUTOVG_API bool plutovg_canvas_set_paint(plutovg_canvas_t* canvas, plutovg_paint_t* paint);

/**
 * @brief Retrieves the current paint.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param color A pointer to a `plutovg_color_t` object where the current color will be stored.
 * @return The current `plutovg_paint_t` used for drawing operations. If no paint is set, `NULL` is returned.
 */
PLUTOVG_API plutovg_paint_t* plutovg_canvas_get_paint(const plutovg_canvas_t* canvas, plutovg_color_t* color);

/**
 * @brief Sets the font face and size for text rendering on the canvas.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param face A pointer to a `plutovg_font_face_t` object representing the font face to use.
 * @param size The size of the font, in pixels. This determines the height of the rendered text.
 */
PLUTOVG_API void plutovg_canvas_set_font(plutovg_canvas_t* canvas, plutovg_font_face_t* face, float size);

/**
 * @brief Sets the font face for text rendering on the canvas.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param face A pointer to a `plutovg_font_face_t` object representing the font face to use.
 */
PLUTOVG_API void plutovg_canvas_set_font_face(plutovg_canvas_t* canvas, plutovg_font_face_t* face);

/**
 * @brief Retrieves the current font face used for text rendering on the canvas.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @return A pointer to a `plutovg_font_face_t` object representing the current font face.
 */
PLUTOVG_API plutovg_font_face_t* plutovg_canvas_get_font_face(const plutovg_canvas_t* canvas);

/**
 * @brief Sets the font size for text rendering on the canvas.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param size The size of the font, in pixels. This value defines the height of the rendered text.
 */
PLUTOVG_API void plutovg_canvas_set_font_size(plutovg_canvas_t* canvas, float size);

/**
 * @brief Retrieves the current font size used for text rendering on the canvas.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @return The current font size, in pixels. This value represents the height of the rendered text.
 */
PLUTOVG_API float plutovg_canvas_get_font_size(const plutovg_canvas_t* canvas);

/**
 * @brief Sets the fill rule.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param winding The fill rule.
 */
PLUTOVG_API void plutovg_canvas_set_fill_rule(plutovg_canvas_t* canvas, plutovg_fill_rule_t winding);

/**
 * @brief Retrieves the current fill rule.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @return The current fill rule.
 */
PLUTOVG_API plutovg_fill_rule_t plutovg_canvas_get_fill_rule(const plutovg_canvas_t* canvas);

/**
 * @brief Sets the compositing operator.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param op The compositing operator.
 */
PLUTOVG_API void plutovg_canvas_set_operator(plutovg_canvas_t* canvas, plutovg_operator_t op);

/**
 * @brief Retrieves the current compositing operator.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @return The current compositing operator.
 */
PLUTOVG_API plutovg_operator_t plutovg_canvas_get_operator(const plutovg_canvas_t* canvas);

/**
 * @brief Sets the global opacity.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param opacity The opacity value (0 to 1).
 */
PLUTOVG_API void plutovg_canvas_set_opacity(plutovg_canvas_t* canvas, float opacity);

/**
 * @brief Retrieves the current global opacity.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @return The current opacity value.
 */
PLUTOVG_API float plutovg_canvas_get_opacity(const plutovg_canvas_t* canvas);

/**
 * @brief Sets the line width.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param line_width The width of the stroke.
 */
PLUTOVG_API void plutovg_canvas_set_line_width(plutovg_canvas_t* canvas, float line_width);

/**
 * @brief Retrieves the current line width.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @return The current line width.
 */
PLUTOVG_API float plutovg_canvas_get_line_width(const plutovg_canvas_t* canvas);

/**
 * @brief Sets the line cap style.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param line_cap The line cap style.
 */
PLUTOVG_API void plutovg_canvas_set_line_cap(plutovg_canvas_t* canvas, plutovg_line_cap_t line_cap);

/**
 * @brief Retrieves the current line cap style.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @return The current line cap style.
 */
PLUTOVG_API plutovg_line_cap_t plutovg_canvas_get_line_cap(const plutovg_canvas_t* canvas);

/**
 * @brief Sets the line join style.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param line_join The line join style.
 */
PLUTOVG_API void plutovg_canvas_set_line_join(plutovg_canvas_t* canvas, plutovg_line_join_t line_join);

/**
 * @brief Retrieves the current line join style.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @return The current line join style.
 */
PLUTOVG_API plutovg_line_join_t plutovg_canvas_get_line_join(const plutovg_canvas_t* canvas);

/**
 * @brief Sets the miter limit.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param miter_limit The miter limit value.
 */
PLUTOVG_API void plutovg_canvas_set_miter_limit(plutovg_canvas_t* canvas, float miter_limit);

/**
 * @brief Retrieves the current miter limit.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @return The current miter limit value.
 */
PLUTOVG_API float plutovg_canvas_get_miter_limit(const plutovg_canvas_t* canvas);

/**
 * @brief Sets the dash pattern.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param offset The dash offset.
 * @param dashes Array of dash lengths.
 * @param ndashes Number of dash lengths.
 */
PLUTOVG_API void plutovg_canvas_set_dash(plutovg_canvas_t* canvas, float offset, const float* dashes, int ndashes);

/**
 * @brief Sets the dash offset.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param offset The dash offset.
 */
PLUTOVG_API void plutovg_canvas_set_dash_offset(plutovg_canvas_t* canvas, float offset);

/**
 * @brief Retrieves the current dash offset.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @return The current dash offset.
 */
PLUTOVG_API float plutovg_canvas_get_dash_offset(const plutovg_canvas_t* canvas);

/**
 * @brief Sets the dash pattern.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param dashes Array of dash lengths.
 * @param ndashes Number of dash lengths.
 */
PLUTOVG_API bool plutovg_canvas_set_dash_array(plutovg_canvas_t* canvas, const float* dashes, int ndashes);

/**
 * @brief Retrieves the current dash pattern.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param dashes Pointer to store the dash array.
 * @return The number of dash lengths.
 */
PLUTOVG_API int plutovg_canvas_get_dash_array(const plutovg_canvas_t* canvas, const float** dashes);

/**
 * @brief Translates the current transformation matrix by offsets `tx` and `ty`.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param tx The translation offset in the x-direction.
 * @param ty The translation offset in the y-direction.
 */
PLUTOVG_API void plutovg_canvas_translate(plutovg_canvas_t* canvas, float tx, float ty);

/**
 * @brief Scales the current transformation matrix by factors `sx` and `sy`.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param sx The scaling factor in the x-direction.
 * @param sy The scaling factor in the y-direction.
 */
PLUTOVG_API void plutovg_canvas_scale(plutovg_canvas_t* canvas, float sx, float sy);

/**
 * @brief Shears the current transformation matrix by factors `shx` and `shy`.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param shx The shearing factor in the x-direction.
 * @param shy The shearing factor in the y-direction.
 */
PLUTOVG_API void plutovg_canvas_shear(plutovg_canvas_t* canvas, float shx, float shy);

/**
 * @brief Rotates the current transformation matrix by the specified angle (in radians).
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param angle The rotation angle in radians.
 */
PLUTOVG_API void plutovg_canvas_rotate(plutovg_canvas_t* canvas, float angle);

/**
 * @brief Multiplies the current transformation matrix with the specified `matrix`.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param matrix A pointer to the `::gfx::matrix` object.
 */
PLUTOVG_API void plutovg_canvas_transform(plutovg_canvas_t* canvas, const ::gfx::matrix* matrix);

/**
 * @brief Resets the current transformation matrix to the identity matrix.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 */
PLUTOVG_API void plutovg_canvas_reset_matrix(plutovg_canvas_t* canvas);

/**
 * @brief Resets the current transformation matrix to the specified `matrix`.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param matrix A pointer to the `::gfx::matrix` object.
 */
PLUTOVG_API void plutovg_canvas_set_matrix(plutovg_canvas_t* canvas, const ::gfx::matrix* matrix);

/**
 * @brief Stores the current transformation matrix in `matrix`.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param A pointer to the `::gfx::matrix` to store the matrix.
 */
PLUTOVG_API void plutovg_canvas_get_matrix(const plutovg_canvas_t* canvas, ::gfx::matrix* matrix);

/**
 * @brief Transforms the point `(x, y)` using the current transformation matrix and stores the result in `(xx, yy)`.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param x The x-coordinate of the point to transform.
 * @param y The y-coordinate of the point to transform.
 * @param xx A pointer to store the transformed x-coordinate.
 * @param yy A pointer to store the transformed y-coordinate.
 */
PLUTOVG_API void plutovg_canvas_map(const plutovg_canvas_t* canvas, float x, float y, float* xx, float* yy);

/**
 * @brief Transforms the `src` point using the current transformation matrix and stores the result in `dst`.
 * @note `src` and `dst` can be identical.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param src A pointer to the `::gfx::pointf` point to transform.
 * @param dst A pointer to the `::gfx::pointf` to store the transformed point.
 */
PLUTOVG_API void plutovg_canvas_map_point(const plutovg_canvas_t* canvas, const ::gfx::pointf* src, ::gfx::pointf* dst);

/**
 * @brief Transforms the `src` rectangle using the current transformation matrix and stores the result in `dst`.
 * @note `src` and `dst` can be identical.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param src A pointer to the `plutovg_rect_t` rectangle to transform.
 * @param dst A pointer to the `plutovg_rect_t` to store the transformed rectangle.
 */
PLUTOVG_API void plutovg_canvas_map_rect(const plutovg_canvas_t* canvas, const plutovg_rect_t* src, plutovg_rect_t* dst);

/**
 * @brief Moves the current point to a new position.
 *
 * Moves the current point to the specified coordinates without adding a line.
 * This operation is added to the current path. Equivalent to the SVG `M` command.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param x The x-coordinate of the new position.
 * @param y The y-coordinate of the new position.
 */
PLUTOVG_API bool plutovg_canvas_move_to(plutovg_canvas_t* canvas, float x, float y);

/**
 * @brief Adds a straight line segment to the current path.
 *
 * Adds a straight line from the current point to the specified coordinates.
 * This segment is added to the current path. Equivalent to the SVG `L` command.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param x The x-coordinate of the end point of the line.
 * @param y The y-coordinate of the end point of the line.
 */
PLUTOVG_API bool plutovg_canvas_line_to(plutovg_canvas_t* canvas, float x, float y);

/**
 * @brief Adds a quadratic Bézier curve to the current path.
 *
 * Adds a quadratic Bézier curve from the current point to the specified end point,
 * using the given control point. This curve is added to the current path. Equivalent to the SVG `Q` command.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param x1 The x-coordinate of the control point.
 * @param y1 The y-coordinate of the control point.
 * @param x2 The x-coordinate of the end point of the curve.
 * @param y2 The y-coordinate of the end point of the curve.
 */
PLUTOVG_API bool plutovg_canvas_quad_to(plutovg_canvas_t* canvas, float x1, float y1, float x2, float y2);

/**
 * @brief Adds a cubic Bézier curve to the current path.
 *
 * Adds a cubic Bézier curve from the current point to the specified end point,
 * using the given control points. This curve is added to the current path. Equivalent to the SVG `C` command.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param x1 The x-coordinate of the first control point.
 * @param y1 The y-coordinate of the first control point.
 * @param x2 The x-coordinate of the second control point.
 * @param y2 The y-coordinate of the second control point.
 * @param x3 The x-coordinate of the end point of the curve.
 * @param y3 The y-coordinate of the end point of the curve.
 */
PLUTOVG_API bool plutovg_canvas_cubic_to(plutovg_canvas_t* canvas, float x1, float y1, float x2, float y2, float x3, float y3);

/**
 * @brief Adds an elliptical arc to the current path.
 *
 * Adds an elliptical arc from the current point to the specified end point,
 * defined by radii, rotation angle, and flags for arc type and direction.
 * This arc segment is added to the current path. Equivalent to the SVG `A` command.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param rx The x-radius of the ellipse.
 * @param ry The y-radius of the ellipse.
 * @param angle The rotation angle of the ellipse in degrees.
 * @param large_arc_flag If true, add the large arc; otherwise, add the small arc.
 * @param sweep_flag If true, add the arc in the positive-angle direction; otherwise, in the negative-angle direction.
 * @param x The x-coordinate of the end point.
 * @param y The y-coordinate of the end point.
 */
PLUTOVG_API bool plutovg_canvas_arc_to(plutovg_canvas_t* canvas, float rx, float ry, float angle, bool large_arc_flag, bool sweep_flag, float x, float y);

/**
 * @brief Adds a rectangle to the current path.
 *
 * Adds a rectangle with the specified position and dimensions to the current path.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param x The x-coordinate of the rectangle's origin.
 * @param y The y-coordinate of the rectangle's origin.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
PLUTOVG_API bool plutovg_canvas_rect(plutovg_canvas_t* canvas, float x, float y, float w, float h);

/**
 * @brief Adds a rounded rectangle to the current path.
 *
 * Adds a rectangle with rounded corners defined by the specified position,
 * dimensions, and corner radii to the current path.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param x The x-coordinate of the rectangle's origin.
 * @param y The y-coordinate of the rectangle's origin.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @param rx The x-radius of the corners.
 * @param ry The y-radius of the corners.
 */
PLUTOVG_API bool plutovg_canvas_round_rect(plutovg_canvas_t* canvas, float x, float y, float w, float h, float rx, float ry);

/**
 * @brief Adds an ellipse to the current path.
 *
 * Adds an ellipse centered at the specified coordinates with the given radii to the current path.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param cx The x-coordinate of the ellipse's center.
 * @param cy The y-coordinate of the ellipse's center.
 * @param rx The x-radius of the ellipse.
 * @param ry The y-radius of the ellipse.
 */
PLUTOVG_API bool plutovg_canvas_ellipse(plutovg_canvas_t* canvas, float cx, float cy, float rx, float ry);

/**
 * @brief Adds a circle to the current path.
 *
 * Adds a circle centered at the specified coordinates with the given radius to the current path.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param cx The x-coordinate of the circle's center.
 * @param cy The y-coordinate of the circle's center.
 * @param r The radius of the circle.
 */
PLUTOVG_API bool plutovg_canvas_circle(plutovg_canvas_t* canvas, float cx, float cy, float r);

/**
 * @brief Adds an arc to the current path.
 *
 * Adds an arc centered at the specified coordinates, with a given radius,
 * starting and ending at the specified angles. The direction of the arc is
 * determined by `ccw`. This arc segment is added to the current path.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param cx The x-coordinate of the arc's center.
 * @param cy The y-coordinate of the arc's center.
 * @param r The radius of the arc.
 * @param a0 The starting angle of the arc in radians.
 * @param a1 The ending angle of the arc in radians.
 * @param ccw If true, add the arc counter-clockwise; otherwise, clockwise.
 */
PLUTOVG_API bool plutovg_canvas_arc(plutovg_canvas_t* canvas, float cx, float cy, float r, float a0, float a1, bool ccw);

/**
 * @brief Adds a path to the current path.
 *
 * Appends the elements of the specified path to the current path.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param path A pointer to the `plutovg_path_t` object to be added.
 */
PLUTOVG_API bool plutovg_canvas_add_path(plutovg_canvas_t* canvas, const plutovg_path_t* path);

/**
 * @brief Starts a new path on the canvas.
 *
 * Begins a new path, clearing any existing path data. The new path starts with no commands.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 */
PLUTOVG_API void plutovg_canvas_new_path(plutovg_canvas_t* canvas);

/**
 * @brief Closes the current path.
 *
 * Closes the current path by adding a straight line back to the starting point.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 */
PLUTOVG_API bool plutovg_canvas_close_path(plutovg_canvas_t* canvas);

/**
 * @brief Retrieves the current point of the canvas.
 *
 * Gets the coordinates of the current point in the canvas, which is the last point
 * added or moved to in the current path.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param x The x-coordinate of the current point.
 * @param y The y-coordinate of the current point.
 */
PLUTOVG_API void plutovg_canvas_get_current_point(const plutovg_canvas_t* canvas, float* x, float* y);

/**
 * @brief Gets the current path from the canvas.
 *
 * Retrieves the path object representing the sequence of path commands added to the canvas.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @return The current path.
 */
PLUTOVG_API plutovg_path_t* plutovg_canvas_get_path(const plutovg_canvas_t* canvas);

/**
 * @brief Gets the bounding box of the filled region.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param extents The bounding box of the filled region.
 */
PLUTOVG_API bool plutovg_canvas_fill_extents(const plutovg_canvas_t* canvas, plutovg_rect_t* extents);

/**
 * @brief Gets the bounding box of the stroked region.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param extents The bounding box of the stroked region.
 */
PLUTOVG_API bool plutovg_canvas_stroke_extents(const plutovg_canvas_t* canvas, plutovg_rect_t* extents);

/**
 * @brief Gets the bounding box of the clipped region.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param extents The bounding box of the clipped region.
 */
PLUTOVG_API void plutovg_canvas_clip_extents(const plutovg_canvas_t* canvas, plutovg_rect_t* extents);

/**
 * @brief A drawing operator that fills the current path according to the current fill rule.
 *
 * The current path will be cleared after this operation.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 */
PLUTOVG_API bool plutovg_canvas_fill(plutovg_canvas_t* canvas,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief A drawing operator that strokes the current path according to the current stroke settings.
 *
 * The current path will be cleared after this operation.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 */
PLUTOVG_API bool plutovg_canvas_stroke(plutovg_canvas_t* canvas,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief A drawing operator that intersects the current clipping region with the current path according to the current fill rule.
 *
 * The current path will be cleared after this operation.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 */
PLUTOVG_API bool plutovg_canvas_clip(plutovg_canvas_t* canvas,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief A drawing operator that paints the current clipping region using the current paint.
 *
 * @note The current path will not be affected by this operation.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 */
PLUTOVG_API bool plutovg_canvas_paint(plutovg_canvas_t* canvas);

/**
 * @brief A drawing operator that fills the current path according to the current fill rule.
 *
 * The current path will be preserved after this operation.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 */
PLUTOVG_API bool plutovg_canvas_fill_preserve(plutovg_canvas_t* canvas,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief A drawing operator that strokes the current path according to the current stroke settings.
 *
 * The current path will be preserved after this operation.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 */
PLUTOVG_API bool plutovg_canvas_stroke_preserve(plutovg_canvas_t* canvas,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief A drawing operator that intersects the current clipping region with the current path according to the current fill rule.
 *
 * The current path will be preserved after this operation.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 */
PLUTOVG_API bool plutovg_canvas_clip_preserve(plutovg_canvas_t* canvas,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief Fills a rectangle according to the current fill rule.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param x The x-coordinate of the rectangle's origin.
 * @param y The y-coordinate of the rectangle's origin.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
PLUTOVG_API bool plutovg_canvas_fill_rect(plutovg_canvas_t* canvas, float x, float y, float w, float h,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief Fills a path according to the current fill rule.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param path The `plutovg_path_t` object.
 */
PLUTOVG_API bool plutovg_canvas_fill_path(plutovg_canvas_t* canvas, const plutovg_path_t* path,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief Strokes a rectangle with the current stroke settings.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param x The x-coordinate of the rectangle's origin.
 * @param y The y-coordinate of the rectangle's origin.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
PLUTOVG_API bool plutovg_canvas_stroke_rect(plutovg_canvas_t* canvas, float x, float y, float w, float h,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief Strokes a path with the current stroke settings.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param path The `plutovg_path_t` object.
 */
PLUTOVG_API bool plutovg_canvas_stroke_path(plutovg_canvas_t* canvas, const plutovg_path_t* path,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief Intersects the current clipping region with a rectangle according to the current fill rule.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param x The x-coordinate of the rectangle's origin.
 * @param y The y-coordinate of the rectangle's origin.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
PLUTOVG_API bool plutovg_canvas_clip_rect(plutovg_canvas_t* canvas, float x, float y, float w, float h,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief Intersects the current clipping region with a path according to the current fill rule.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param path The `plutovg_path_t` object.
 */
PLUTOVG_API bool plutovg_canvas_clip_path(plutovg_canvas_t* canvas, const plutovg_path_t* path,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief Adds a glyph to the current path at the specified origin.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param codepoint The glyph codepoint.
 * @param x The x-coordinate of the origin.
 * @param y The y-coordinate of the origin.
 * @return The advance width of the glyph.
 */
PLUTOVG_API float plutovg_canvas_add_glyph(plutovg_canvas_t* canvas, plutovg_codepoint_t codepoint, float x, float y);

/**
 * @brief Adds text to the current path at the specified origin.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param text The text data.
 * @param length The length of the text data, or -1 if null-terminated.
 * @param encoding The encoding of the text data.
 * @param x The x-coordinate of the origin.
 * @param y The y-coordinate of the origin.
 * @return The total advance width of the text.
 */
PLUTOVG_API float plutovg_canvas_add_text(plutovg_canvas_t* canvas, const ::gfx::text_handle text, size_t length, const ::gfx::text_encoder* encoding, float x, float y);

/**
 * @brief Fills a text at the specified origin.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param text The text data.
 * @param length The length of the text data, or -1 if null-terminated.
 * @param encoding The encoding of the text data.
 * @param x The x-coordinate of the origin.
 * @param y The y-coordinate of the origin.
 * @return The total advance width of the text.
 */
PLUTOVG_API float plutovg_canvas_fill_text(plutovg_canvas_t* canvas, const ::gfx::text_handle text, size_t length, const ::gfx::text_encoder* encoding, float x, float y,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief Strokes a text at the specified origin.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param text The text data.
 * @param length The length of the text data, or -1 if null-terminated.
 * @param encoding The encoding of the text data.
 * @param x The x-coordinate of the origin.
 * @param y The y-coordinate of the origin.
 * @return The total advance width of the text.
 */
PLUTOVG_API float plutovg_canvas_stroke_text(plutovg_canvas_t* canvas, const ::gfx::text_handle text, size_t length, const ::gfx::text_encoder* encoding, float x, float y,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief Intersects the current clipping region with text at the specified origin.
 *
 * @note The current path will be cleared by this operation.
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param text The text data.
 * @param length The length of the text data, or -1 if null-terminated.
 * @param encoding The encoding of the text data.
 * @param x The x-coordinate of the origin.
 * @param y The y-coordinate of the origin.
 * @return The total advance width of the text.
 */
PLUTOVG_API float plutovg_canvas_clip_text(plutovg_canvas_t* canvas, const ::gfx::text_handle text, size_t length, const ::gfx::text_encoder* encoding, float x, float y,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*));

/**
 * @brief Retrieves font metrics for the current font.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param ascent The ascent of the font.
 * @param descent The descent of the font.
 * @param line_gap The line gap of the font.
 * @param extents The bounding box of the font.
 */
PLUTOVG_API void plutovg_canvas_font_metrics(plutovg_canvas_t* canvas, float* ascent, float* descent, float* line_gap, plutovg_rect_t* extents);

/**
 * @brief Retrieves metrics for a specific glyph.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param codepoint The glyph codepoint.
 * @param advance_width The advance width of the glyph.
 * @param left_side_bearing The left side bearing of the glyph.
 * @param extents The bounding box of the glyph.
 */
PLUTOVG_API void plutovg_canvas_glyph_metrics(plutovg_canvas_t* canvas, plutovg_codepoint_t codepoint, float* advance_width, float* left_side_bearing, plutovg_rect_t* extents);

/**
 * @brief Retrieves the extents of a text.
 *
 * @param canvas A pointer to a `plutovg_canvas_t` object.
 * @param text The text data.
 * @param length The length of the text data, or -1 if null-terminated.
 * @param encoding The encoding of the text data.
 * @param extents The bounding box of the text.
 * @return The total advance width of the text.
 */
PLUTOVG_API float plutovg_canvas_text_extents(plutovg_canvas_t* canvas, const void* text, int length, plutovg_text_encoding_t encoding, plutovg_rect_t* extents);

#ifdef __cplusplus
}
#endif

#endif // PLUTOVG_H
