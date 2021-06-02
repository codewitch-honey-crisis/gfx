#define HTCW_LITTLE_ENDIAN
#include <stdio.h>
#include <string.h>
#include <gfx_bitmap.hpp>
#include <gfx_drawing.hpp>
#include <gfx_color_cpp14.hpp>
#include "../fonts/terminal.h"
using namespace gfx;

#define PATH_CHAR '/'

// prints a bitmap as 4-bit grayscale ASCII
template <typename BitmapType>
void dump_bitmap(const BitmapType& bmp) {
    static const char *col_table = " .,-~;*+!=1%O@$#";
    using gsc4 = pixel<channel_traits<channel_name::L,4>>;
    for(int y = 0;y<bmp.dimensions().height;++y) {
        for(int x = 0;x<bmp.dimensions().width;++x) {
            typename BitmapType::pixel_type px;
            bmp.point(point16(x,y),&px);
            const auto px2 = convert<typename BitmapType::pixel_type,gsc4>(px);
            size_t i =px2.template channel<0>();
            printf("%c",col_table[i]);
            
        }
        printf("\r\n");
    }
}

int main(int argc, char** argv) {

    // use whatever bit depth you like. 
    // the only difference really is that
    // it might fudge the colors a bit
    // if you go down really low
    // and that can change the ascii
    // output, although with what
    // we draw below it can be as 
    // low as 3 with what we draw
    // below without impacting
    // anything. Byte aligned things
    // are quicker, so multiples of 8
    // are good.
    static const size_t bit_depth = 24;
    
    // our type definitions
    // this makes things easier
    using bmp_type = bitmap<rgb_pixel<bit_depth>>;
    using color = color<typename bmp_type::pixel_type>;

    static const size16 bmp_size(32,32);
    
    // declare the bitmap
    uint8_t bmp_buf[bmp_type::sizeof_buffer(bmp_size)];
    bmp_type bmp(bmp_size,bmp_buf);

    // draw stuff
    bmp.clear(bmp.bounds()); // comment this out and check out the uninitialized RAM. It looks neat.

    // bounding info for the face
    srect16 bounds(0,0,bmp_size.width-1,(bmp_size.height-1)/(4/3.0));
    rect16 ubounds(0,0,bounds.x2,bounds.y2);

    // draw the face
    draw::filled_ellipse(bmp,bounds,color::yellow);
    
    // draw the left eye
    srect16 eye_bounds_left(spoint16(bounds.width()/5,bounds.height()/5),ssize16(bounds.width()/5,bounds.height()/3));
    draw::filled_ellipse(bmp,eye_bounds_left,color::black);
    
    // draw the right eye
    srect16 eye_bounds_right(
        spoint16(
            bmp_size.width-eye_bounds_left.x1-eye_bounds_left.width(),
            eye_bounds_left.y1
        ),eye_bounds_left.dimensions());
    draw::filled_ellipse(bmp,eye_bounds_right,color::black);
    
    // draw the mouth
    srect16 mouth_bounds=bounds.inflate(-bounds.width()/7,-bounds.height()/8).normalize();
    // we need to clip part of the circle we'll be drawing
    srect16 mouth_clip(mouth_bounds.x1,mouth_bounds.y1+mouth_bounds.height()/(float)1.6,mouth_bounds.x2,mouth_bounds.y2);
    draw::ellipse(bmp,mouth_bounds,color::black,&mouth_clip);

    // now blt the bitmaps
    const size_t count  =3; // 3 faces
    // our second bitmap. Not strictly necessary because one
    // can draw to the same bitmap they copy from
    // but doing so can cause garbage if the regions
    // overlap. This way is safer but takes more RAM
    using bmp2_type = bitmap<typename bmp_type::pixel_type>;
    static const size16 bmp2_size(128,64);
    uint8_t buf2[bmp2_type::sizeof_buffer(bmp2_size)];
    bmp2_type bmp2(bmp2_size,buf2);

    // if we don't do the following we'll get uninitialized garbage.
    // it looks neat though:
    bmp2.clear(bmp2.bounds());
    srect16 r = bounds;
    // how much to shrink each iteration:
    spoint16 shrink(-bounds.width()/8,-bounds.height()/8);
    for(size_t i = 0;i<count;++i) {
        // draw the bitmap
        draw::bitmap(bmp2,r,bmp,ubounds,bitmap_resize::resize_bilinear);
        // move the rect, flip and shrink it
        r=r.offset(r.width(),0);
        r=r.inflate(shrink.x,shrink.y);
        // rect orientation dictates bitmap orientation
        r=r.flip_vertical();     
    }
    // we can load fonts from a file, but that requires heap
    // while PROGMEM arrays do not (at least on devices
    // that use flash memory)
    // io::file_stream dynfs("./fonts/Bm437_ToshibaSat_8x8.FON");
    // font dynf(&dynfs);
    // store the rightmost extent
    // so we have something to 
    // center by
    int er = r.right();
    // now let's draw some text
    // choose the font you want to use below:
    const font& f =
        terminal_fon // use embedded font
        // dynf // use dynamic font
    ;

    const char* str = "Have a nice day!";
    // create the bounding rectangle for our 
    // proposed text
    r=srect16(0,bounds.height(),er+1,bmp2_size.height);
    // now "shrink" the bounding rectangle to our
    // actual text size:
    r=srect16(r.top_left(),f.measure_text(r.dimensions(),str));
    // center the bounding rect:
    int16_t s = (er+1)-r.width();
    if(0>s)
        s=0;
    r=r.offset(s/2,0);
    
    // now draw
    draw::text(bmp2,r,str,f,color::white);

    // display our mess
    dump_bitmap(bmp2);
    return 0;
}