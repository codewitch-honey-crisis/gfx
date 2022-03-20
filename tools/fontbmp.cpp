// define if windows:
#define WINDOWS
#define HTCW_LITTLE_ENDIAN
#include <stdio.h>
#include <string.h>
#include <gfx_cpp14.hpp>
using namespace gfx;
#ifdef WINDOWS
#define PATH_CHAR '\\'
#else
#define PATH_CHAR '/'
#endif
const char* get_fname(const char* filepath) {
    
    const char* sz=filepath;
    while(nullptr!=sz && *sz) {
        sz=strchr(filepath,PATH_CHAR);
        if(sz!=nullptr) {
            filepath=sz+1;
        } 
    }
    return filepath;
}
void make_ident(const char* name,char* buffer) {
    if((*name>='A' && *name<='Z')||(*name>='a' && *name<='z')||('_'==*name)) {
        *buffer=*name;
    } else
        *buffer = '_';
    ++name;
    ++buffer;
    while(*name) {
        if((*name>='A' && *name<='Z')||(*name>='a' && *name<='z')||(*name>='0'&&*name<='9')||('_'==*name)) {
            *buffer=*name;
        } else
            *buffer = '_';
        ++name;    
        ++buffer;
    }
    *buffer='\0';
}
void print_esc_char(char ch) {
    if(ch>=' ' && ch<='~' && ch!='\"' && ch!='\'') {
        printf("'%c'",ch);
        return ;
    }
    switch(ch) {
        case 0:
            printf("'\\0'");
            return;
        case '\'':
            printf("'\\\''");
            return;
        case '\"':
            printf("'\\\"'");
            return;
        case '\t':
            printf("'\\t'");
            return;
        case '\r':
            printf("'\\r'");
            return;
        case '\a':
            printf("'\\a'");
            return;
        case '\b':
            printf("'\\b'");
            return;
        case '\f':
            printf("'\\f'");
            return;
        case '\v':
            printf("'\\v'");
            return;
        default:
            printf("'\\x%02X'",(uint8_t)ch);
            return;
    }
}
 // from libxml http://xmlsoft.org/
   int latin1_to_utf8(unsigned char* out, size_t outlen, const unsigned char* in, size_t inlen)
   {
      unsigned char* outstart= out;
      unsigned char* outend= out+outlen;
      const unsigned char* inend= in+inlen;
      unsigned char c;

      while (in < inend) {
         c= *in++;
         if (c < 0x80) {
               if (out >= outend)  return -1;
               *out++ = c;
               return 1;
         }
         else {
               if (out >= outend)  return -1;
               *out++ = 0xC0 | (c >> 6);
               if (out >= outend)  return -1;
               *out++ = 0x80 | (0x3F & c);
               return 2;
         }
      }
      return out-outstart;
   }
   // from libxml http://xmlsoft.org/
   int utf8_to_utf16(uint16_t* out, size_t outlen, const unsigned char* in, size_t inlen)
   {
      uint16_t* outstart= out;
      uint16_t* outend= out+outlen;
      const unsigned char* inend= in+inlen;
      unsigned int c, d, trailing;

      while (in < inend) {
         d= *in++;
         if      (d < 0x80)  { c= d; trailing= 0; }
         else if (d < 0xC0)  return -2;    /* trailing byte in leading position */
         else if (d < 0xE0)  { c= d & 0x1F; trailing= 1; }
         else if (d < 0xF0)  { c= d & 0x0F; trailing= 2; }
         else if (d < 0xF8)  { c= d & 0x07; trailing= 3; }
         else return -2;    /* no chance for this in UTF-16 */

         for ( ; trailing; trailing--) {
            if ((in >= inend) || (((d= *in++) & 0xC0) != 0x80))  return -1;
            c <<= 6;
            c |= d & 0x3F;
         }

         /* assertion: c is a single UTF-4 value */
         if (c < 0x10000) {
               if (out >= outend)  return -1;
               *out++ = c;
               return 1;
         }
         else if (c < 0x110000) {
               if (out+1 >= outend)  return -1;
               c -= 0x10000;
               *out++ = 0xD800 | (c >> 10);
               *out++ = 0xDC00 | (c & 0x03FF);
               return 2;
         }
         else  return -1;
      }
      return out-outstart;
   }
   int to_utf32_codepoint(const char* in,size_t in_length, int* codepoint, gfx_encoding encoding) {
      int c;
      uint16_t out_tmp[4];
      switch(encoding) {
         case gfx_encoding::utf8: {
            c = utf8_to_utf16(out_tmp,4,(const unsigned char*)in,in_length);
         }
         break;
         case gfx_encoding::latin1: {
            unsigned char out_tmp1[4];
            c = latin1_to_utf8(out_tmp1,4,(const unsigned char*)in,in_length);
            if(c<0) {
               *codepoint = 0;
               return c;
            }
            c=utf8_to_utf16(out_tmp,4,out_tmp1,4);
         }
         break;
         default:
            c=0;
            break;
      }
      switch(c) {
         case 1:
            *codepoint = out_tmp[0];
            break;
         case 2:
            *codepoint = (out_tmp[0] << 10) + out_tmp[1] - 0x35fdc00;
            break;
         default:
            *codepoint = 0;
            break;
      }
      return c;
   }
const char* next_char(const char* sz,int* pcp) {
    int c = to_utf32_codepoint(sz,4,pcp,gfx_encoding::utf8);
    if(c<0) {
        return nullptr;
    }
    return sz+c;
}

#define OUT(x) fprintf(handle,x)
#define OUTLN(x) OUT(x); OUT("\r\n")
void generate_array(const char* ident,const char* name,const uint8_t* data,size_t len, FILE* handle) {
    OUT("static const uint8_t ");
    OUT(ident);
    OUT("_0x");
    OUT(name);
    OUTLN("_bmp_data[] PROGMEM = {");
    OUT("\t");
    const uint8_t* begin = data;
    const uint8_t* end = data+len;
    while(data!=end) {
        for(int i = 0;i<25;++i) {
            if(data!=begin) { 
                OUT(", ");
            }
            fprintf(handle,"0x%02X",(int)*data);
            if(++data==end) break;
        }
        OUTLN("");
        OUT("\t");
    }
    OUTLN("};");
    
}
int generate(const open_font& f,const char* fname,const char* chars, int height, const int forecolor,int backcolor, int fmt,FILE* handle) {
    char buf[5];
    char ident[1024];
    make_ident(fname,ident);
    OUT("// Bitmapped font header for ");
    OUTLN(fname);
    OUTLN("// Generated by the fontbmp tool");
    OUTLN("#ifndef PROGMEM\r\n\t#define PROGMEM\r\n#endif\r\n\r\n");
    OUTLN("#include <gfx_bitmap.hpp>");
    OUTLN("");
    float scale = f.scale(height);
    int cp;
    const char* sz = chars;
    while(sz && *sz) {
        const char* nsz = next_char(sz,&cp);
        if(nsz==nullptr) nsz = sz+strlen(sz);
        memcpy(buf,sz,nsz-sz);
        buf[nsz-sz]=0;
        size16 bsz = (size16)f.measure_text({32767,32767},{0,0},buf,scale);
        rgb_pixel<24> sfg,sbg;
        sfg.channel<0>(uint8_t((forecolor>>16)&0xFF));
        sfg.channel<1>(uint8_t((forecolor>>8)&0xFF));
        sfg.channel<2>(uint8_t((forecolor)&0xFF));
        sbg.channel<0>(uint8_t((backcolor>>16)&0xFF));
        sbg.channel<1>(uint8_t((backcolor>>8)&0xFF));
        sbg.channel<2>(uint8_t((backcolor)&0xFF));
        switch(fmt) {
            case 0: {
                using bmp_type = bitmap<rgb_pixel<16>>;
                const size_t bmp_len = bmp_type::sizeof_buffer(bsz);
                uint8_t* bmp_buf = (uint8_t*)malloc(bmp_len);
                if(bmp_buf==nullptr) {
                    return (int)gfx_result::out_of_memory;
                }
                bmp_type bmp(bsz,bmp_buf);
                draw::filled_rectangle(bmp,(srect16)bmp.bounds(),sbg);
                draw::text(bmp,(srect16)bmp.bounds(),{0,0},buf,f,scale,sfg,sbg,false);
                char n[64];
                generate_array(ident,itoa(cp,n,16),bmp_buf,bmp_len,handle);
                free(bmp_buf);

                OUT("static const gfx::const_bitmap<gfx::rgb_pixel<16>> ");
                OUT(ident);
                OUT("_0x");
                OUT(n);
                OUT("_bmp({");
                OUT(itoa(bmp.dimensions().width,n,10));
                OUT(", ");
                OUT(itoa(bmp.dimensions().height,n,10));
                OUT("}, ");
                OUT(ident);
                OUT("_0x");
                OUT(itoa(cp,n,16));
                OUTLN("_bmp_data);");
                
            }
            break;
            case 1: {
                using bmp_type = bitmap<gsc_pixel<4>>;
                const size_t bmp_len = bmp_type::sizeof_buffer(bsz);
                uint8_t* bmp_buf = (uint8_t*)malloc(bmp_len);
                if(bmp_buf==nullptr) {
                    return (int)gfx_result::out_of_memory;
                }
                bmp_type bmp(bsz,bmp_buf);
                draw::filled_rectangle(bmp,(srect16)bmp.bounds(),sbg);
                draw::text(bmp,(srect16)bmp.bounds(),{0,0},buf,f,scale,sfg,sbg,false);
                char n[64];
                generate_array(ident,itoa(cp,n,16),bmp_buf,bmp_len,handle);
                free(bmp_buf);

                OUT("static const gfx::const_bitmap<gfx::gsc_pixel<4>> ");
                OUT(ident);
                OUT("_0x");
                OUT(n);
                OUT("_bmp({");
                OUT(itoa(bmp.dimensions().width,n,10));
                OUT(", ");
                OUT(itoa(bmp.dimensions().height,n,10));
                OUT("}, ");
                OUT(ident);
                OUT("_0x");
                OUT(itoa(cp,n,16));
                OUTLN("_bmp_data);");
                
            }
            break;
            case 2: {
                using bmp_type = bitmap<gsc_pixel<1>>;
                const size_t bmp_len = bmp_type::sizeof_buffer(bsz);
                uint8_t* bmp_buf = (uint8_t*)malloc(bmp_len);
                if(bmp_buf==nullptr) {
                    return (int)gfx_result::out_of_memory;
                }
                bmp_type bmp(bsz,bmp_buf);
                draw::filled_rectangle(bmp,(srect16)bmp.bounds(),sbg);
                draw::text(bmp,(srect16)bmp.bounds(),{0,0},buf,f,scale,sfg,sbg,false);
                char n[64];
                generate_array(ident,itoa(cp,n,16),bmp_buf,bmp_len,handle);
                free(bmp_buf);

                OUT("static const gfx::const_bitmap<gfx::gsc_pixel<1>> ");
                OUT(ident);
                OUT("_0x");
                OUT(n);
                OUT("_bmp({");
                OUT(itoa(bmp.dimensions().width,n,10));
                OUT(", ");
                OUT(itoa(bmp.dimensions().height,n,10));
                OUT("}, ");
                OUT(ident);
                OUT("_0x");
                OUT(itoa(cp,n,16));
                OUTLN("_bmp_data);");
                
            }
            break;
            OUTLN("");
        }
        sz=nsz;
    }
    OUTLN("");
    OUT("static const gfx::const_bitmap<gfx::");
    switch(fmt) {
        case 0:
            OUT("rgb_pixel<16>");
            break;
        case 1:
            OUT("gsc_pixel<4>");
            break;
        case 2:
            OUT("gsc_pixel<1>");
            break;
    }
    OUT("> ");
    OUT(ident);
    OUTLN("_bmps[] = {");
    sz = chars;
    while(sz && *sz) {
        const char* nsz = next_char(sz,&cp);
        if(nsz==nullptr) nsz = sz+strlen(sz);
        memcpy(buf,sz,nsz-sz);
        buf[nsz-sz]=0;
        OUT("\t");
        if(sz!=chars) {
            OUT(",");
        }
        OUT(ident);
        OUT("_0x");
        char n[64];
        OUT(itoa(cp,n,16));
        OUTLN("_bmp");
        sz=nsz;
    }
    OUTLN("\t};");
    

    
    return 0;
}
void print_usage() {
    fprintf(stderr,"USAGE: fontbmp <inputFile> <characters> <height> <forecolor> <backcolor> <format> [<outputFile>]\r\n\r\n");
    fprintf(stderr,"<inputFile>    The font file to use\r\n");
    fprintf(stderr,"<characters>   A string of characters to render\r\n");
    fprintf(stderr,"<height>       The character height in pixels\r\n");
    fprintf(stderr,"<forecolor>    The forecolor in 24-bit hex, like FFFFFF for white\r\n");
    fprintf(stderr,"<backcolor>    The backcolor in 24-bit hex\r\n");
    fprintf(stderr,"<format>       The format. Either RGB, GSC or MONO\r\n");
    fprintf(stderr,"<outputFile>   The header file to generate. Defaults to STDOUT\r\n");
    fprintf(stderr,"\r\n");

}
/*int test(int argc, char** argv) {
    const char* ifile = ".\\tools\\test.ttf";
    auto stm = io::file_stream(ifile,io::file_mode::read);
    open_font f;
    if(gfx_result::success!= open_font::open(&stm,&f)) {
        printf("Error");
        return -1;
    }
    return generate(f,"test.ttf","0123456789",24,0x0000FF,0xFFFFFF,0,stdout);
}*/
int main(int argc, char** argv) {
    
    if(argc!=7 && argc!=8) {
        print_usage();
        fprintf(stderr,"Wrong number of arguments. Should be 6.\r\n\r\n");
        return (int)gfx_result::invalid_argument;
    }   
    const char* ifile = argv[1]; 
    fprintf(stderr,"Generating %s...\r\n",ifile);
    
    auto stm = io::file_stream(ifile,io::file_mode::read);
            
    open_font f;
    auto r= open_font::open(&stm,&f);
    if(gfx_result::success==r) {
        const char* fname = get_fname(argv[1]);
        const char* chars = argv[2];
        const int height = atoi(argv[3]);
        const int forecolor = strtol(argv[4],nullptr,16);
        const int backcolor = strtol(argv[5],nullptr,16);
        int fmt = -1;
        if(0==strcmp("rgb",argv[6]) || 0==strcmp("RGB",argv[6])) {
            fmt = 0;
        }
        if(0==strcmp("gsc",argv[6]) || 0==strcmp("GSC",argv[6])) {
            fmt = 1;
        }
        if(0==strcmp("mono",argv[6]) || 0==strcmp("MONO",argv[6])) {
            fmt = 2;
        }
        if(0>fmt) {
            print_usage();
            fprintf(stderr,"Invalid format. Should be RGB, GSC, or MONO.\r\n\r\n");
        return (int)gfx_result::invalid_argument;
        }
        FILE* handle = stdout;
        if(argc==8) {
            handle=fopen(argv[7],"w");
        }
        return generate(f,fname,chars,height,forecolor,backcolor,fmt,handle);
        if(handle!=stdout) {
            fclose(handle);
        }
    } else {
        fprintf(stderr,"Failed with %d\r\n",(int)r);
        return (int)r;
    }

}