#ifndef HTCW_GFX_MATH_HPP
#define HTCW_GFX_MATH_HPP
#include <stdint.h>
/* Derived from LVGL:
MIT licence
Copyright (c) 2021 LVGL Kft

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
namespace gfx {
namespace helpers {
    struct math_consts {
        constexpr static const int TRIGO_SIN_MAX = 32767;
        constexpr static const int TRIGO_SHIFT = 15; /**<  >> LV_TRIGO_SHIFT to normalize*/

        constexpr static const int BEZIER_VAL_MAX = 1024; /**< Max time in Bezier functions (not [0..1] to use integers)*/
        constexpr static const int BEZIER_VAL_SHIFT = 10; /**< log2(LV_BEZIER_VAL_MAX): used to normalize up scaled values*/
    };
}

struct gfx_sqrt_result final {
    uint16_t i;
    uint16_t f;
} ;

struct math {
    constexpr static const float pi =      3.14159265358979323846f;
    constexpr static const float two_pi =  6.28318530717958647693f;
    constexpr static const float half_pi = 1.57079632679489661923f;
    constexpr static const float sqrt2 =   1.41421356237309504880f;
    constexpr static const float kappa =   0.55228474983079339840f;

    static int16_t sin(int16_t angle);
    static int16_t cos(int16_t angle);
    static uint32_t bezier3(uint32_t t, uint32_t u0, uint32_t u1, uint32_t u2, uint32_t u3);
    static uint16_t atan2(int x, int y);
    static void sqrt(uint32_t x, gfx_sqrt_result * q, uint32_t mask);
    static int64_t pow(int64_t base, int8_t exp);
    static int32_t map(int32_t x, int32_t min_in, int32_t max_in, int32_t min_out, int32_t max_out);
    static uint32_t rand(uint32_t min, uint32_t max);
    
    static inline float deg2rad(float x) {
        return x * (pi/180.0f);
    }
    static inline float rad2deg(float x) {
        return x * (180.0f/pi);
    }
    template<typename T> constexpr static T min_(T a, T b) {
        return a < b ? a : b;
    }
    template<typename T> constexpr static T min_(T a, T b, T c) {
        return min_(min_(a,b),c);
    }
    template<typename T> constexpr static T min_(T a, T b, T c, T d) {
        return min_(min_(a,b),min_(c,d));
    }
    template<typename T> constexpr static T max_(T a, T b) {
        return a > b ? a : b;
    }
    template<typename T> constexpr static T max_(T a, T b, T c) {
        return max_(max_(a,b),c);
    }
    template<typename T> constexpr static T max_(T a, T b, T c, T d) {
        return max_(max_(a,b),max_(c,d));
    }
    template<typename T> constexpr static T clamp(T minimum, T value, T maximum) {
        return max_(minimum, (min_(value, maximum)));
    }
    template<typename T> constexpr static T absx(T value) {
        return value > 0 ? value : -value;
    }
    template<typename T> constexpr static T udiv255(T value) {
        return ((value) * 0x8081U) >> 0x17;
    }

    template<typename T> constexpr static bool is_signed() {
        return ((T)-1) < ((T)-0);
    }
    template<typename T> constexpr static T umax_of() {
        return ((0x1ULL << ((sizeof(T) * 8ULL) - 1ULL)) - 1ULL) | (0xFULL << ((sizeof(T) * 8ULL) - 4ULL));
    }
    template<typename T> constexpr static T smax_of() {
        return ((0x1ULL << ((sizeof(T) * 8ULL) - 1ULL)) - 1ULL) | (0x7ULL << ((sizeof(T) * 8ULL) - 4ULL));
    }
    template<typename T> constexpr static T max_of() {
        return is_signed<T>()?smax_of<T>():umax_of<T>();
    }
    // Approximate sqrt(x*x+y*y) using the `alpha max plus beta min'
    // algorithm.  We use alpha = 1, beta = 3/8, giving us results with a
    // largest error less than 7% compared to the exact value.
    template<typename T> constexpr static T hypot(T x, T y) {
        T xx = (T)absx(x);
        T yy = (T)absx(y);
        return xx>yy? xx+(3*yy>>3):yy+(3*xx>>3);
    }
    constexpr static const int32_t pi_angle_ft16_16 = ( 180L << 16 );
    constexpr static const int32_t twopi_angle_ft16_16 = pi_angle_ft16_16 * 2;
    constexpr static const int32_t pitwo_angle_ft16_16 = pi_angle_ft16_16 / 2;
    constexpr static const int32_t pifour_angle_ft16_16 = pi_angle_ft16_16 / 4;
    // fixed point multiply
    static int32_t mul_ft16_16( int32_t  a, int32_t  b );
    static int32_t muldiv_ft16_16(int32_t a, int32_t b, int32_t c);
    static int32_t div_ft16_16(int32_t a, int32_t b);
    static int32_t sin_ft16_16(int32_t a);
    static int32_t cos_ft16_16(int32_t a);
    static int32_t tan_ft16_16(int32_t a);
    static int32_t atan2_ft16_16(int32_t x, int32_t y);
    static int32_t angle_diff_ft16_16(int32_t lhs,int32_t rhs);
    static void vector_unit_ft16_16(int32_t angle, int32_t* out_x,int32_t* out_y);
    static void vector_rotate_ft16_16(int32_t angle, int32_t* in_out_x,int32_t* in_out_y);
    static int32_t vector_length_ft16_16(int32_t x,int32_t y);
    static void vector_polarize_ft16_16(int32_t x,int32_t y, int32_t* out_length, int32_t* out_angle);
    static void vector_from_polar_ft16_16(int32_t length,int32_t angle, int32_t* out_x,int32_t* out_y);
};
}
#endif
