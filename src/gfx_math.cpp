#include <gfx_math.hpp>
#include <stdint.h>
using namespace gfx;

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

static const int16_t sin0_90_table[] = {
    0,     572,   1144,  1715,  2286,  2856,  3425,  3993,  4560,  5126,  5690,  6252,  6813,  7371,  7927,  8481,
    9032,  9580,  10126, 10668, 11207, 11743, 12275, 12803, 13328, 13848, 14364, 14876, 15383, 15886, 16383, 16876,
    17364, 17846, 18323, 18794, 19260, 19720, 20173, 20621, 21062, 21497, 21925, 22347, 22762, 23170, 23571, 23964,
    24351, 24730, 25101, 25465, 25821, 26169, 26509, 26841, 27165, 27481, 27788, 28087, 28377, 28659, 28932, 29196,
    29451, 29697, 29934, 30162, 30381, 30591, 30791, 30982, 31163, 31335, 31498, 31650, 31794, 31927, 32051, 32165,
    32269, 32364, 32448, 32523, 32587, 32642, 32687, 32722, 32747, 32762, 32767
};

int16_t math::sin(int16_t angle)
{
    int16_t ret = 0;
    angle       = angle % 360;

    if(angle < 0) angle = 360 + angle;

    if(angle < 90) {
        ret = sin0_90_table[angle];
    }
    else if(angle >= 90 && angle < 180) {
        angle = 180 - angle;
        ret   = sin0_90_table[angle];
    }
    else if(angle >= 180 && angle < 270) {
        angle = angle - 180;
        ret   = -sin0_90_table[angle];
    }
    else {   /*angle >=270*/
        angle = 360 - angle;
        ret   = -sin0_90_table[angle];
    }

    return ret;
}
int16_t math::cos(int16_t angle) {
    return sin(angle+90);
}
uint32_t math::bezier3(uint32_t t, uint32_t u0, uint32_t u1, uint32_t u2, uint32_t u3)
{
    uint32_t t_rem  = 1024 - t;
    uint32_t t_rem2 = (t_rem * t_rem) >> 10;
    uint32_t t_rem3 = (t_rem2 * t_rem) >> 10;
    uint32_t t2     = (t * t) >> 10;
    uint32_t t3     = (t2 * t) >> 10;

    uint32_t v1 = (t_rem3 * u0) >> 10;
    uint32_t v2 = (3 * t_rem2 * t * u1) >> 20;
    uint32_t v3 = (3 * t_rem * t2 * u2) >> 20;
    uint32_t v4 = (t3 * u3) >> 10;

    return v1 + v2 + v3 + v4;
}

void math::sqrt(uint32_t x, gfx_sqrt_result * q, uint32_t mask)
{
    x = x << 8; /*To get 4 bit precision. (sqrt(256) = 16 = 4 bit)*/

    uint32_t root = 0;
    uint32_t trial;
    // http://ww1.microchip.com/...en/AppNotes/91040a.pdf
    do {
        trial = root + mask;
        if(trial * trial <= x) root = trial;
        mask = mask >> 1;
    } while(mask);

    q->i = root >> 4;
    q->f = (root & 0xf) << 4;
}
uint16_t math::atan2(int x, int y)
{
    // Fast XY vector to integer degree algorithm - Jan 2011 www.RomanBlack.com
    // Converts any XY values including 0 to a degree value that should be
    // within +/- 1 degree of the accurate value without needing
    // large slow trig functions like ArcTan() or ArcCos().
    // NOTE! at least one of the X or Y values must be non-zero!
    // This is the full version, for all 4 quadrants and will generate
    // the angle in integer degrees from 0-360.
    // Any values of X and Y are usable including negative values provided
    // they are between -1456 and 1456 so the 16bit multiply does not overflow.

    unsigned char negflag;
    unsigned char tempdegree;
    unsigned char comp;
    unsigned int degree;     // this will hold the result
    unsigned int ux;
    unsigned int uy;

    // Save the sign flags then remove signs and get XY as unsigned ints
    negflag = 0;
    if(x < 0) {
        negflag += 0x01;    // x flag bit
        x = (0 - x);        // is now +
    }
    ux = x;                // copy to unsigned var before multiply
    if(y < 0) {
        negflag += 0x02;    // y flag bit
        y = (0 - y);        // is now +
    }
    uy = y;                // copy to unsigned var before multiply

    // 1. Calc the scaled "degrees"
    if(ux > uy) {
        degree = (uy * 45) / ux;   // degree result will be 0-45 range
        negflag += 0x10;    // octant flag bit
    }
    else {
        degree = (ux * 45) / uy;   // degree result will be 0-45 range
    }

    // 2. Compensate for the 4 degree error curve
    comp = 0;
    tempdegree = degree;    // use an unsigned char for speed!
    if(tempdegree > 22) {    // if top half of range
        if(tempdegree <= 44) comp++;
        if(tempdegree <= 41) comp++;
        if(tempdegree <= 37) comp++;
        if(tempdegree <= 32) comp++;  // max is 4 degrees compensated
    }
    else {   // else is lower half of range
        if(tempdegree >= 2) comp++;
        if(tempdegree >= 6) comp++;
        if(tempdegree >= 10) comp++;
        if(tempdegree >= 15) comp++;  // max is 4 degrees compensated
    }
    degree += comp;   // degree is now accurate to +/- 1 degree!

    // Invert degree if it was X>Y octant, makes 0-45 into 90-45
    if(negflag & 0x10) degree = (90 - degree);

    // 3. Degree is now 0-90 range for this quadrant,
    // need to invert it for whichever quadrant it was in
    if(negflag & 0x02) { // if -Y
        if(negflag & 0x01)   // if -Y -X
            degree = (180 + degree);
        else        // else is -Y +X
            degree = (180 - degree);
    }
    else {   // else is +Y
        if(negflag & 0x01)   // if +Y -X
            degree = (360 - degree);
    }
    return degree;
}

int64_t math::pow(int64_t base, int8_t exp)
{
    int64_t result = 1;
    while(exp) {
        if(exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }

    return result;
}

int32_t math::map(int32_t x, int32_t min_in, int32_t max_in, int32_t min_out, int32_t max_out)
{
    if(max_in >= min_in && x >= max_in) return max_out;
    if(max_in >= min_in && x <= min_in) return min_out;

    if(max_in <= min_in && x <= max_in) return max_out;
    if(max_in <= min_in && x >= min_in) return min_out;

    /**
     * The equation should be:
     *   ((x - min_in) * delta_out) / delta in) + min_out
     * To avoid rounding error reorder the operations:
     *   (x - min_in) * (delta_out / delta_min) + min_out
     */

    int32_t delta_in = max_in - min_in;
    int32_t delta_out = max_out - min_out;

    return ((x - min_in) * delta_out) / delta_in + min_out;
}

uint32_t math::rand(uint32_t min, uint32_t max)
{
    static uint32_t a = 0x1234ABCD; /*Seed*/

    /*Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs"*/
    uint32_t x = a;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    a = x;

    return (a % (max - min + 1)) + min;
}
int32_t math::mul_ft16_16( int32_t  a, int32_t  b ) {
    int32_t  s = 1;
    int32_t c;

    PVG_FT_MOVE_SIGN(a, s);
    PVG_FT_MOVE_SIGN(b, s);

    c = (int32_t)(((int64_t)a * b + 0x8000L) >> 16);

    return (s > 0) ? c : -c;
}
int32_t math::muldiv_ft16_16(int32_t a, int32_t b, int32_t c) {
    int32_t  s = 1;
    int32_t d;

    PVG_FT_MOVE_SIGN(a, s);
    PVG_FT_MOVE_SIGN(b, s);
    PVG_FT_MOVE_SIGN(c, s);

    d = (int32_t)(c > 0 ? ((int64_t)a * b + (c >> 1)) / c : 0x7FFFFFFFL);

    return (s > 0) ? d : -d;
}
int32_t math::div_ft16_16(int32_t a, int32_t b) {
    int32_t  s = 1;
    int32_t q;

    PVG_FT_MOVE_SIGN(a, s);
    PVG_FT_MOVE_SIGN(b, s);

    q = (int32_t)(b > 0 ? (((int64_t)a << 16) + (b >> 1)) / b
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

static const int32_t ft_trig_arctan_table[] = {
    1740967L, 919879L, 466945L, 234379L, 117304L, 58666L, 29335L, 14668L,
    7334L,    3667L,   1833L,   917L,    458L,    229L,   115L,   57L,
    29L,      14L,     7L,      4L,      2L,      1L};

/* multiply a given value by the CORDIC shrink factor */
static int32_t ft_trig_downscale(int32_t val)
{
    int32_t s;
    int64_t v;

    s = val;
    val = math::absx(val);

    v = (val * (int64_t)PVG_FT_TRIG_SCALE) + 0x100000000UL;
    val = (int32_t)(v >> 32);

    return (s >= 0) ? val : -val;
}

/* undefined and never called for zero vector */
static int32_t ft_trig_prenorm(int32_t* in_out_x, int32_t* in_out_y)
{
    int32_t shift;
    int32_t x = *in_out_x;
    int32_t y = *in_out_y;
    shift = PVG_FT_MSB(math::absx(x) | math::absx(y));

    if (shift <= PVG_FT_TRIG_SAFE_MSB) {
        shift = PVG_FT_TRIG_SAFE_MSB - shift;
        *in_out_x = (int32_t)((int32_t)x << shift);
        *in_out_y = (int32_t)((int32_t)y << shift);
    } else {
        shift -= PVG_FT_TRIG_SAFE_MSB;
        *in_out_x = x >> shift;
        *in_out_y = y >> shift;
        shift = -shift;
    }

    return shift;
}

static void ft_trig_pseudo_rotate(int32_t* in_out_x, int32_t* in_out_y, int32_t theta)
{
    int32_t          i;
    int32_t        x, y, xtemp, b;
    const int32_t* arctanptr;

    x = *in_out_x;
    y = *in_out_y;

    /* Rotate inside [-PI/4,PI/4] sector */
    while (theta < -math::pifour_angle_ft16_16) {
        xtemp = y;
        y = -x;
        x = xtemp;
        theta += math::pitwo_angle_ft16_16;
    }

    while (theta > math::pifour_angle_ft16_16) {
        xtemp = -y;
        y = x;
        x = xtemp;
        theta -= math::pitwo_angle_ft16_16;
    }

    arctanptr = ft_trig_arctan_table;

    /* Pseudorotations, with right shifts */
    for (i = 1, b = 1; i < PVG_FT_TRIG_MAX_ITERS; b <<= 1, i++) {
        int32_t v1 = ((y + b) >> i);
        int32_t v2 = ((x + b) >> i);
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

    *in_out_x = x;
    *in_out_y = y;
}

static void ft_trig_pseudo_polarize(int32_t* in_out_x, int32_t* in_out_y)
{
    int32_t        theta;
    int32_t          i;
    int32_t        x, y, xtemp, b;
    const int32_t* arctanptr;

    x = *in_out_x;
    y = *in_out_y;

    /* Get the vector into [-PI/4,PI/4] sector */
    if (y > x) {
        if (y > -x) {
            theta = math::pitwo_angle_ft16_16;
            xtemp = y;
            y = -x;
            x = xtemp;
        } else {
            theta = y > 0 ? math::pi_angle_ft16_16 : -math::pi_angle_ft16_16;
            x = -x;
            y = -y;
        }
    } else {
        if (y < -x) {
            theta = -math::pitwo_angle_ft16_16;
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
        int32_t v1 = ((y + b) >> i);
        int32_t v2 = ((x + b) >> i);
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

    *in_out_x = x;
    *in_out_y = theta;
}

int32_t math::sin_ft16_16(int32_t a) {
    return math::cos_ft16_16(math::pitwo_angle_ft16_16-a);
}
int32_t math::cos_ft16_16(int32_t a) {
    int32_t vx,vy;

    vx = PVG_FT_TRIG_SCALE >> 8;
    vy = 0;
    ft_trig_pseudo_rotate(&vx,&vy, a);

    return (vx + 0x80L) >> 8;
}
int32_t math::tan_ft16_16(int32_t a) {
    int32_t vx,vy;
    vx = PVG_FT_TRIG_SCALE >> 8;
    vy = 0;
    ft_trig_pseudo_rotate(&vx,&vy, a);

    return div_ft16_16(vy, vx);
}
int32_t math::atan2_ft16_16(int32_t dx, int32_t dy) {
    int32_t vx,vy;
    if (dx == 0 && dy == 0) return 0;

    vx = dx;
    vy = dy;
    ft_trig_prenorm(&vx,&vy);
    ft_trig_pseudo_polarize(&vx,&vy);

    return vy;
}
int32_t math::angle_diff_ft16_16(int32_t angle1,int32_t angle2) {
    int32_t delta = angle2 - angle1;

    while ( delta <= -math::pi_angle_ft16_16 )
        delta += math::twopi_angle_ft16_16;

    while ( delta > math::pi_angle_ft16_16 )
        delta -= math::twopi_angle_ft16_16;

    return delta;
}
void math::vector_unit_ft16_16(int32_t angle, int32_t* out_x,int32_t* out_y) {
    *out_x = PVG_FT_TRIG_SCALE >> 8;
    *out_y = 0;
    ft_trig_pseudo_rotate(out_x,out_y, angle);
    *out_x = (*out_x + 0x80L) >> 8;
    *out_y = (*out_y + 0x80L) >> 8;
}
void math::vector_rotate_ft16_16(int32_t angle, int32_t* in_out_x,int32_t* in_out_y) {
    int32_t shift;
    int32_t vx,vy;
    vx=*in_out_x;
    vy=*in_out_y;

    if ( vx == 0 && vy == 0 )
        return;

    shift = ft_trig_prenorm( &vx,&vy );
    ft_trig_pseudo_rotate( &vx,&vy, angle );
    vx = ft_trig_downscale( vx );
    vy = ft_trig_downscale( vy );

    if ( shift > 0 )
    {
        int32_t  half = (int32_t)1L << ( shift - 1 );


        *in_out_x = ( vx + half - ( vx < 0 ) ) >> shift;
        *in_out_y = ( vy + half - ( vy < 0 ) ) >> shift;
    }
    else
    {
        shift  = -shift;
        *in_out_x = (int32_t)( (uint32_t)vx << shift );
        *in_out_y = (int32_t)( (uint32_t)vy << shift );
    } 
}
int32_t math::vector_length_ft16_16(int32_t x,int32_t y) {
    int32_t    shift;
    int32_t vx=x,vy=y;

    /* handle trivial cases */
    if (vx == 0) {
        return math::absx(vy);
    } else if (vy == 0) {
        return math::absx(vx);
    }

    /* general case */
    shift = ft_trig_prenorm(&vx,&vy);
    ft_trig_pseudo_polarize(&vx,&vy);

    vx = ft_trig_downscale(vx);

    if (shift > 0) return (vx + (1 << (shift - 1))) >> shift;

    return (int32_t)((uint32_t)vx << -shift);
}
void math::vector_polarize_ft16_16(int32_t x,int32_t y, int32_t* out_length, int32_t* out_angle) {
    int32_t    shift;
    int32_t vx=x,vy=y;

    if (vx == 0 && vy == 0) return;

    shift = ft_trig_prenorm(&vx,&vy);
    ft_trig_pseudo_polarize(&vx,&vy);

    vx = ft_trig_downscale(vx);

    *out_length = (shift >= 0) ? (vx >> shift)
                           : (int32_t)((uint32_t)vx << -shift);
    *out_angle = vy;
}
void math::vector_from_polar_ft16_16(int32_t length,int32_t angle, int32_t* out_x,int32_t* out_y) {
    *out_x = length;
    *out_y = 0;

    math::vector_rotate_ft16_16(angle,out_x,out_y);
}