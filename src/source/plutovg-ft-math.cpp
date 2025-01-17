/***************************************************************************/
/*                                                                         */
/*  fttrigon.c                                                             */
/*                                                                         */
/*    FreeType trigonometric functions (body).                             */
/*                                                                         */
/*  Copyright 2001-2005, 2012-2013 by                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, FTL.txt.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/

#include "plutovg-ft-math.h"

#if defined(_MSC_VER)
#include <intrin.h>
static inline int clz(unsigned int x) {
    unsigned long r = 0;
    if (_BitScanReverse(&r, x))
        return 31 - r;
    return 32;
}
#define PVG_FT_MSB(x)  (31 - clz(x))
#elif defined(__GNUC__)
#define PVG_FT_MSB(x)  (31 - __builtin_clz(x))
#else
static inline int clz(unsigned int x) {
    for (int i = 31; i >= 0; i--)
    {
        if (x >> i)
        {
            return 31 - i;
        }
    }

    return 32;
}
#define PVG_FT_MSB(x)  (31 - clz(x))
#endif

#define PVG_FT_PAD_FLOOR(x, n) ((x) & ~((n)-1))
#define PVG_FT_PAD_ROUND(x, n) PVG_FT_PAD_FLOOR((x) + ((n) / 2), n)
#define PVG_FT_PAD_CEIL(x, n) PVG_FT_PAD_FLOOR((x) + ((n)-1), n)

#define PVG_FT_BEGIN_STMNT do {
#define PVG_FT_END_STMNT } while (0)

/* transfer sign leaving a positive number */
#define PVG_FT_MOVE_SIGN(x, s) \
    PVG_FT_BEGIN_STMNT         \
    if (x < 0) {              \
        x = -x;               \
        s = -s;               \
    }                         \
    PVG_FT_END_STMNT

vector_long_t PVG_FT_MulFix(vector_long_t a, vector_long_t b)
{
    vector_int_t  s = 1;
    vector_long_t c;

    PVG_FT_MOVE_SIGN(a, s);
    PVG_FT_MOVE_SIGN(b, s);

    c = (vector_long_t)(((vector_int64_t)a * b + 0x8000L) >> 16);

    return (s > 0) ? c : -c;
}

vector_long_t PVG_FT_MulDiv(vector_long_t a, vector_long_t b, vector_long_t c)
{
    vector_int_t  s = 1;
    vector_long_t d;

    PVG_FT_MOVE_SIGN(a, s);
    PVG_FT_MOVE_SIGN(b, s);
    PVG_FT_MOVE_SIGN(c, s);

    d = (vector_long_t)(c > 0 ? ((vector_int64_t)a * b + (c >> 1)) / c : 0x7FFFFFFFL);

    return (s > 0) ? d : -d;
}

vector_long_t PVG_FT_DivFix(vector_long_t a, vector_long_t b)
{
    vector_int_t  s = 1;
    vector_long_t q;

    PVG_FT_MOVE_SIGN(a, s);
    PVG_FT_MOVE_SIGN(b, s);

    q = (vector_long_t)(b > 0 ? (((vector_uint64_t)a << 16) + (b >> 1)) / b
                           : 0x7FFFFFFFL);

    return (s < 0 ? -q : q);
}

/*************************************************************************/
/*                                                                       */
/* This is a fixed-point CORDIC implementation of trigonometric          */
/* functions as well as transformations between Cartesian and polar      */
/* coordinates.  The angles are represented as 16.16 fixed-point values  */
/* in degrees, i.e., the angular resolution is 2^-16 degrees.  Note that */
/* only vectors longer than 2^16*180/pi (or at least 22 bits) on a       */
/* discrete Cartesian grid can have the same or better angular           */
/* resolution.  Therefore, to maintain this precision, some functions    */
/* require an interim upscaling of the vectors, whereas others operate   */
/* with 24-bit long vectors directly.                                    */
/*                                                                       */
/*************************************************************************/

/* the Cordic shrink factor 0.858785336480436 * 2^32 */
#define PVG_FT_TRIG_SCALE 0xDBD95B16UL

/* the highest bit in overflow-safe vector components, */
/* MSB of 0.858785336480436 * sqrt(0.5) * 2^30         */
#define PVG_FT_TRIG_SAFE_MSB 29

/* this table was generated for PVG_FT_PI = 180L << 16, i.e. degrees */
#define PVG_FT_TRIG_MAX_ITERS 23

static const vector_fixed_t ft_trig_arctan_table[] = {
    1740967L, 919879L, 466945L, 234379L, 117304L, 58666L, 29335L, 14668L,
    7334L,    3667L,   1833L,   917L,    458L,    229L,   115L,   57L,
    29L,      14L,     7L,      4L,      2L,      1L};

/* multiply a given value by the CORDIC shrink factor */
static vector_fixed_t ft_trig_downscale(vector_fixed_t val)
{
    vector_fixed_t s;
    vector_int64_t v;

    s = val;
    val = PVG_FT_ABS(val);

    v = (val * (vector_int64_t)PVG_FT_TRIG_SCALE) + 0x100000000UL;
    val = (vector_fixed_t)(v >> 32);

    return (s >= 0) ? val : -val;
}

/* undefined and never called for zero vector */
static vector_int_t ft_trig_prenorm(vector_t* vec)
{
    vector_pos_t x, y;
    vector_int_t shift;

    x = vec->x;
    y = vec->y;

    shift = PVG_FT_MSB(PVG_FT_ABS(x) | PVG_FT_ABS(y));

    if (shift <= PVG_FT_TRIG_SAFE_MSB) {
        shift = PVG_FT_TRIG_SAFE_MSB - shift;
        vec->x = (vector_pos_t)((vector_ulong_t)x << shift);
        vec->y = (vector_pos_t)((vector_ulong_t)y << shift);
    } else {
        shift -= PVG_FT_TRIG_SAFE_MSB;
        vec->x = x >> shift;
        vec->y = y >> shift;
        shift = -shift;
    }

    return shift;
}

static void ft_trig_pseudo_rotate(vector_t* vec, PVG_FT_Angle theta)
{
    vector_int_t          i;
    vector_fixed_t        x, y, xtemp, b;
    const vector_fixed_t* arctanptr;

    x = vec->x;
    y = vec->y;

    /* Rotate inside [-PI/4,PI/4] sector */
    while (theta < -PVG_FT_ANGLE_PI4) {
        xtemp = y;
        y = -x;
        x = xtemp;
        theta += PVG_FT_ANGLE_PI2;
    }

    while (theta > PVG_FT_ANGLE_PI4) {
        xtemp = -y;
        y = x;
        x = xtemp;
        theta -= PVG_FT_ANGLE_PI2;
    }

    arctanptr = ft_trig_arctan_table;

    /* Pseudorotations, with right shifts */
    for (i = 1, b = 1; i < PVG_FT_TRIG_MAX_ITERS; b <<= 1, i++) {
        vector_fixed_t v1 = ((y + b) >> i);
        vector_fixed_t v2 = ((x + b) >> i);
        if (theta < 0) {
            xtemp = x + v1;
            y = y - v2;
            x = xtemp;
            theta += *arctanptr++;
        } else {
            xtemp = x - v1;
            y = y + v2;
            x = xtemp;
            theta -= *arctanptr++;
        }
    }

    vec->x = x;
    vec->y = y;
}

static void ft_trig_pseudo_polarize(vector_t* vec)
{
    PVG_FT_Angle        theta;
    vector_int_t          i;
    vector_fixed_t        x, y, xtemp, b;
    const vector_fixed_t* arctanptr;

    x = vec->x;
    y = vec->y;

    /* Get the vector into [-PI/4,PI/4] sector */
    if (y > x) {
        if (y > -x) {
            theta = PVG_FT_ANGLE_PI2;
            xtemp = y;
            y = -x;
            x = xtemp;
        } else {
            theta = y > 0 ? PVG_FT_ANGLE_PI : -PVG_FT_ANGLE_PI;
            x = -x;
            y = -y;
        }
    } else {
        if (y < -x) {
            theta = -PVG_FT_ANGLE_PI2;
            xtemp = -y;
            y = x;
            x = xtemp;
        } else {
            theta = 0;
        }
    }

    arctanptr = ft_trig_arctan_table;

    /* Pseudorotations, with right shifts */
    for (i = 1, b = 1; i < PVG_FT_TRIG_MAX_ITERS; b <<= 1, i++) {
        vector_fixed_t v1 = ((y + b) >> i);
        vector_fixed_t v2 = ((x + b) >> i);
        if (y > 0) {
            xtemp = x + v1;
            y = y - v2;
            x = xtemp;
            theta += *arctanptr++;
        } else {
            xtemp = x - v1;
            y = y + v2;
            x = xtemp;
            theta -= *arctanptr++;
        }
    }

    /* round theta */
    if (theta >= 0)
        theta = PVG_FT_PAD_ROUND(theta, 32);
    else
        theta = -PVG_FT_PAD_ROUND(-theta, 32);

    vec->x = x;
    vec->y = theta;
}

/* documentation is in fttrigon.h */

vector_fixed_t PVG_FT_Cos(PVG_FT_Angle angle)
{
    vector_t v;

    v.x = PVG_FT_TRIG_SCALE >> 8;
    v.y = 0;
    ft_trig_pseudo_rotate(&v, angle);

    return (v.x + 0x80L) >> 8;
}

/* documentation is in fttrigon.h */

vector_fixed_t PVG_FT_Sin(PVG_FT_Angle angle)
{
    return PVG_FT_Cos(PVG_FT_ANGLE_PI2 - angle);
}

/* documentation is in fttrigon.h */

vector_fixed_t PVG_FT_Tan(PVG_FT_Angle angle)
{
    vector_t v;

    v.x = PVG_FT_TRIG_SCALE >> 8;
    v.y = 0;
    ft_trig_pseudo_rotate(&v, angle);

    return PVG_FT_DivFix(v.y, v.x);
}

/* documentation is in fttrigon.h */

PVG_FT_Angle PVG_FT_Atan2(vector_fixed_t dx, vector_fixed_t dy)
{
    vector_t v;

    if (dx == 0 && dy == 0) return 0;

    v.x = dx;
    v.y = dy;
    ft_trig_prenorm(&v);
    ft_trig_pseudo_polarize(&v);

    return v.y;
}

/* documentation is in fttrigon.h */

void PVG_FT_Vector_Unit(vector_t* vec, PVG_FT_Angle angle)
{
    vec->x = PVG_FT_TRIG_SCALE >> 8;
    vec->y = 0;
    ft_trig_pseudo_rotate(vec, angle);
    vec->x = (vec->x + 0x80L) >> 8;
    vec->y = (vec->y + 0x80L) >> 8;
}

void PVG_FT_Vector_Rotate(vector_t* vec, PVG_FT_Angle angle)
{
    vector_int_t     shift;
    vector_t  v = *vec;

    if ( v.x == 0 && v.y == 0 )
        return;

    shift = ft_trig_prenorm( &v );
    ft_trig_pseudo_rotate( &v, angle );
    v.x = ft_trig_downscale( v.x );
    v.y = ft_trig_downscale( v.y );

    if ( shift > 0 )
    {
        vector_int32_t  half = (vector_int32_t)1L << ( shift - 1 );


        vec->x = ( v.x + half - ( v.x < 0 ) ) >> shift;
        vec->y = ( v.y + half - ( v.y < 0 ) ) >> shift;
    }
    else
    {
        shift  = -shift;
        vec->x = (vector_pos_t)( (vector_ulong_t)v.x << shift );
        vec->y = (vector_pos_t)( (vector_ulong_t)v.y << shift );
    }
}

/* documentation is in fttrigon.h */

vector_fixed_t PVG_FT_Vector_Length(vector_t* vec)
{
    vector_int_t    shift;
    vector_t v;

    v = *vec;

    /* handle trivial cases */
    if (v.x == 0) {
        return PVG_FT_ABS(v.y);
    } else if (v.y == 0) {
        return PVG_FT_ABS(v.x);
    }

    /* general case */
    shift = ft_trig_prenorm(&v);
    ft_trig_pseudo_polarize(&v);

    v.x = ft_trig_downscale(v.x);

    if (shift > 0) return (v.x + (1 << (shift - 1))) >> shift;

    return (vector_fixed_t)((vector_uint32_t)v.x << -shift);
}

/* documentation is in fttrigon.h */

void PVG_FT_Vector_Polarize(vector_t* vec, vector_fixed_t* length,
    PVG_FT_Angle* angle)
{
    vector_int_t    shift;
    vector_t v;

    v = *vec;

    if (v.x == 0 && v.y == 0) return;

    shift = ft_trig_prenorm(&v);
    ft_trig_pseudo_polarize(&v);

    v.x = ft_trig_downscale(v.x);

    *length = (shift >= 0) ? (v.x >> shift)
                           : (vector_fixed_t)((vector_uint32_t)v.x << -shift);
    *angle = v.y;
}

/* documentation is in fttrigon.h */

void PVG_FT_Vector_From_Polar(vector_t* vec, vector_fixed_t length,
    PVG_FT_Angle angle)
{
    vec->x = length;
    vec->y = 0;

    PVG_FT_Vector_Rotate(vec, angle);
}

/* documentation is in fttrigon.h */

PVG_FT_Angle PVG_FT_Angle_Diff( PVG_FT_Angle  angle1, PVG_FT_Angle  angle2 )
{
    PVG_FT_Angle  delta = angle2 - angle1;

    while ( delta <= -PVG_FT_ANGLE_PI )
        delta += PVG_FT_ANGLE_2PI;

    while ( delta > PVG_FT_ANGLE_PI )
        delta -= PVG_FT_ANGLE_2PI;

    return delta;
}

/* END */
