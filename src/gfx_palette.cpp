#include <gfx_palette.hpp>
namespace gfx {
    namespace helpers {
        const unsigned char dither_threshold_map_array[8*8] PROGMEM = {
                    0,48,12,60, 3,51,15,63,
                    32,16,44,28,35,19,47,31,
                    8,56, 4,52,11,59, 7,55,
                    40,24,36,20,43,27,39,23,
                    2,50,14,62, 1,49,13,61,
                    34,18,46,30,33,17,45,29,
                    10,58, 6,54, 9,57, 5,53,
                    42,26,38,22,41,25,37,21 };
        const unsigned char* dither_color::threshold_map = dither_threshold_map_array;

    
    #define d(x) x/64.0
        const double dither_threshold_map_fast_array[8*8] = {
            d( 0), d(48), d(12), d(60), d( 3), d(51), d(15), d(63),
            d(32), d(16), d(44), d(28), d(35), d(19), d(47), d(31),
            d( 8), d(56), d( 4), d(52), d(11), d(59), d( 7), d(55),
            d(40), d(24), d(36), d(20), d(43), d(27), d(39), d(23),
            d( 2), d(50), d(14), d(62), d( 1), d(49), d(13), d(61),
            d(34), d(18), d(46), d(30), d(33), d(17), d(45), d(29),
            d(10), d(58), d( 6), d(54), d( 9), d(57), d( 5), d(53),
            d(42), d(26), d(38), d(22), d(41), d(25), d(37), d(21) };
    #undef d

        const double* dither_color::threshold_map_fast = dither_threshold_map_fast_array;

        double dither_color::color_compare(int r1,int g1,int b1, int r2,int g2,int b2)
        {
            double luma1 = (r1*299 + g1*587 + b1*114) / (255.0*1000);
            double luma2 = (r2*299 + g2*587 + b2*114) / (255.0*1000);
            double lumadiff = luma1-luma2;
            double diffR = (r1-r2)/255.0, diffG = (g1-g2)/255.0, diffB = (b1-b2)/255.0;
            return (diffR*diffR*0.299 + diffG*diffG*0.587 + diffB*diffB*0.114)*0.75
                + lumadiff*lumadiff;
        }
        unsigned* dither_color::dither_luminosity_cache=nullptr;
        
        gfx_result dither_color::unprepare() {
            if(nullptr!=dither_luminosity_cache) {
                delete[] dither_luminosity_cache;
                dither_luminosity_cache = nullptr;
            }
            return gfx_result::success;
        }
        double dither_color::mixing_error(int r,int g,int b,
                            int r0,int g0,int b0,
                            int r1,int g1,int b1,
                            int r2,int g2,int b2,
                            double ratio) {
            return color_compare(r,g,b, r0,g0,b0) 
                + color_compare(r1,g1,b1, r2,g2,b2) * 0.1 * (fabs(ratio-0.5)+0.5);
        }
    }
}
