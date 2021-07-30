#define HTCW_LITTLE_ENDIAN
#include <gfx_cpp14.hpp>
//#include "../fonts/Shangar.h"
//#include "../fonts/Maziro.h"
#include "../fonts/Maziro.h"
using namespace gfx;
using bmp_type = bitmap<gsc_pixel<4>>;

// prints a bitmap as 4-bit grayscale ASCII
template <typename Source>
void dump_source(const Source& src) {
    static const char *col_table = " .,-~;*+!=1%O@$#";
    using gsc4 = pixel<channel_traits<channel_name::L,4>>;
    for(int y = 0;y<src.dimensions().height;++y) {
        for(int x = 0;x<src.dimensions().width;++x) {
            typename Source::pixel_type px;
            src.point(point16(x,y),&px);
            const auto px2 = convert<typename Source::pixel_type,gsc4>(px);
            size_t i =px2.template channel<0>();
            printf("%c",col_table[i]);
            
        }
        printf("\r\n");
    }
}

int main(int argc,char** argv) {
    //const char *path = "D:\\Users\\gazto\\Documents\\gfx\\reference\\Shangar.ttf";
   //const char *path = "D:\\Users\\gazto\\Documents\\gfx\\reference\\Robinette.ttf";
   //const char *path = "C:\\Windows\\Fonts\\l_10646.ttf";
    /*file_stream fs(path);
    if(!fs.caps().read) {
        printf("Can't find file\r\n");
        return -1;
    }
    */
    //open_font& f = Shangar_ttf;
    open_font& f = Maziro_ttf;
    //open_font& f = Orthodoxy_ttf;
    /*gfx_result gr=open_font::open(&fs,&f);
    if(gfx_result::success!= gr) {
        printf("failed to open font (%d)\r\n",(int)gr);
        return -1;
    }*/
    const char* text = "GQP";
    float scale = f.scale(60);
    ssize16 sz = f.measure_text({1024,1024},{5,-7},text,scale).inflate(20,0);
    
    bmp_type bmp((size16)sz,malloc(bmp_type::sizeof_buffer((size16)sz)));
    printf("size (%d,%d)\r\n",(int)sz.width,(int)sz.height);
    bmp.fill(bmp.bounds(), gsc_pixel<4>(true,.1));
    draw::text(bmp,(srect16)bmp.bounds(),{5,-7},text,f,scale, color_max::white);

    dump_source(bmp);
    // free the bitmap
    free(bmp.begin());

    // all we need to do to free the font is close the stream
    //fs.close();
    return 0;
}
