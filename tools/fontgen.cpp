// define if windows:
#define WINDOWS
#define HTCW_LITTLE_ENDIAN
#include <stdio.h>
#include <string.h>
#include <gfx_font.hpp>
#include <gfx_open_font.hpp>
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

void generate(const font& f,const char* filename) {
    char ident[256];
    make_ident(get_fname(filename),ident);
    printf("#ifndef %s_HPP\r\n",ident);
    printf("#define %s_HPP\r\n",ident);
    printf("#include <stdint.h>\r\n");
    printf("#include <gfx_font.hpp>\r\n\r\n");
    printf("#ifndef PROGMEM\r\n\t#define PROGMEM\r\n#endif\r\n\r\n");
    printf("static const uint8_t %s_char_data[] PROGMEM = {\r\n\t",ident);
    size_t cc=0;
    for(size_t i = (uint8_t)f.first_char();i<(uint8_t)f.last_char();++i) {
        font_char fc = f[(char)i];
        uint16_t wb=bits::from_le(fc.width());
        if(i!=(uint8_t)f.first_char())
            printf(", ");
        printf("0x%02X",((uint8_t*)&wb)[0]);
        ++cc;
        if(0==cc%25) {
            printf("\r\n\t");
        }
        printf(", 0x%02X",((uint8_t*)&wb)[1]);
        ++cc;
        if(0==cc%25) {
            printf("\r\n\t");
        }
        wb = (fc.width()+7)/8;
        const uint8_t*b = fc.data();
        for(size_t y=0;y<f.height();++y) {
            for(size_t ccb=0;ccb<wb;++ccb) {
                printf(", 0x%02X",*b);
                ++b;
                ++cc;
                if(0==cc%25) {
                    printf("\r\n\t");
                }
            }
        }
    }
    printf("};\r\n\r\n");
    printf("static const ::gfx::font %s(\r\n\t",ident);
    printf("%d,\r\n\t",(int)f.height());
    printf("%d,\r\n\t",(int)f.average_width());
    printf("%d,\r\n\t",(int)f.point_size());
    printf("%d,\r\n\t",(int)f.ascent());
    printf("::gfx::point16(%d, %d),\r\n\t",(int)f.resolution().x,(int)f.resolution().y);
    print_esc_char(f.first_char());
    printf(",\r\n\t");
    print_esc_char(f.last_char());
    printf(",\r\n\t");
    print_esc_char(f.default_char());
    printf(",\r\n\t");
    print_esc_char(f.break_char());
    printf(",\r\n\t");
    printf("{ ");
    printf("%d, ",(int)f.style().italic);
    printf("%d, ",(int)f.style().underline);
    printf("%d }",(int)f.style().strikeout);
    printf(",\r\n\t");
    printf("%d,\r\n\t",(int)f.weight());
    printf("%d,\r\n\t",(int)f.charset());
    printf("%d,\r\n\t",(int)f.internal_leading());
    printf("%d,\r\n\t",(int)f.external_leading());
    
    printf("%s_char_data);\r\n\r\n",ident);
    printf("#endif // %s_HPP\r\n",ident);
}
void generate(file_stream& f,const char* filename) {
    char ident[256];
    make_ident(get_fname(filename),ident);
    printf("#ifndef %s_HPP\r\n",ident);
    printf("#define %s_HPP\r\n",ident);
    printf("#include <stdint.h>\r\n");
    printf("#include <gfx_core.hpp>\r\n\r\n");
    printf("#include <gfx_open_font.hpp>\r\n\r\n");
    printf("#ifndef PROGMEM\r\n\t#define PROGMEM\r\n#endif\r\n\r\n");
    printf("static const uint8_t %s_char_data[] PROGMEM = {\r\n\t",ident);
    int cc=0;
    size_t len = f.seek(0,seek_origin::end)+1;
    f.seek(0,seek_origin::start);
    for(size_t i = 0;i<len; ++i) {
        if(i!=0)
            printf(", ");
        uint8_t b;
        f.read(&b,1);
        printf("0x%02X",(int)b);
        ++cc;
        if(0==cc%25) {
            printf("\r\n\t");
        }
    }
    printf("};\r\n\r\n");
    
    printf("static ::gfx::const_buffer_stream %s_char_stream(\r\n\t",ident);
    printf("%s_char_data,%d);\r\n\r\n",ident,(int)len);
    printf("static ::gfx::open_font %s(&%s_char_stream);\r\n",ident,ident);
    printf("#endif // %s_HPP\r\n",ident);
}
int main(int argc, char** argv) {
    
    if(argc<2) {
        fprintf(stderr,"Input font file not specified");
        return (int)font::result::invalid_argument;
    }    
    fprintf(stderr,"// %s\r\n",argv[1]);
    if(argc>5) {
        fprintf(stderr,"Too many arguments specified");
        return (int)font::result::invalid_argument;
    }
    size_t index = 0;
    if(argc>2) {
        index = (size_t)atoi(argv[2]);
    }
    char fc='\0',lc='\xFF';
    if(argc>3) {
        fc = sscanf(argv[3],"%c",&fc);
    }
    if(argc>4) {
        fc = sscanf(argv[4],"%c",&lc);
    }
    size_t slen = strlen(argv[1]);
    bool is_open_font = true;
    if(slen>4) {
        const char* sz = argv[1];
        sz+=slen-4;
        if(*sz=='.') {
            ++sz;
            if(*sz=='f' || *sz=='F') {
                ++sz;
                if(*sz=='o' || *sz=='O') {
                    ++sz;
                    if(*sz=='n' || *sz=='N') {
                        is_open_font = false;
                    }
                }
            }

        }
    }
    
    
    auto stm = io::file_stream(argv[1],io::file_mode::read);
            
    if(!is_open_font) {   
        font f;
        auto r = font::read(&stm,&f,index,fc,lc,nullptr);
        
        if(font::result::success==r) {
            generate(f,argv[1]);
        } else {
            fprintf(stderr,"Failed with %d\r\n",(int)r);
            return (int)r;
        }
    } else {
        open_font f;
        auto r= open_font::open(&stm,&f);
        if(gfx_result::success==r) {
            generate(stm,argv[1]);
        } else {
            fprintf(stderr,"Failed with %d\r\n",(int)r);
            return (int)r;
        }
    }
    return 0;
}
#include "shim.hpp"