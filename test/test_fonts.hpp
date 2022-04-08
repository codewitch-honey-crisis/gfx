#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unity.h>
#include <gfx_cpp14.hpp>
#include "test_helpers.hpp"
#include "Bm437_Acer_VGA_8x8.h"
#include "Bm437_Acer_VGA_8x8_FON.h"
#include "Maziro.h"
gfx::ssize16 measure_text(const gfx::font& f,
    gfx::ssize16 dest_size,
    const char* text,
    unsigned int tab_width) {
    gfx::ssize16 result(0,0);
    if(nullptr==text || 0==*text)
        return result;
    const char*sz=text;
    int cw;
    uint16_t rw;
    gfx::srect16 chr(gfx::spoint16(0,0),gfx::ssize16(f.width(*sz),f.height()));
    gfx::font_char fc = f[*sz];
    while(*sz) {
        switch(*sz) {
            case '\r':
                chr.x1=0;
                ++sz;
                if(*sz) {
                    gfx::font_char nfc = f[*sz];
                    chr.x2=chr.x1+nfc.width()-1;
                    fc=nfc;
                }  
                if(chr.x2>=result.width)
                    result.width=chr.x2+1;
                if(chr.y2>=result.height)
                    result.height=chr.y2+1;    
                break;
            case '\n':
                chr.x1=0;
                ++sz;
                if(*sz) {
                    gfx::font_char nfc = f[*sz];
                    chr.x2=chr.x1+nfc.width()-1;
                    fc=nfc;
                }
                chr=chr.offset(0,f.height());
                if(chr.y2>=dest_size.height) {    
                    return dest_size;
                }
                break;
            case '\t':
                ++sz;
                if(*sz) {
                    gfx::font_char nfc = f[*sz];
                    rw=chr.x1+nfc.width()-1;
                    fc=nfc;
                } else
                    rw=chr.width();
                cw = f.average_width()*tab_width;
                chr.x1=((chr.x1/cw)+1)*cw;
                chr.x2=chr.x1+rw-1;
                if(chr.right()>=dest_size.width) {
                    chr.x1=0;
                    chr=chr.offset(0,f.height());
                } 
                if(chr.x2>=result.width)
                    result.width=chr.x2+1;
                if(chr.y2>=result.height)
                    result.height=chr.y2+1;    
                if(chr.y2>=dest_size.height)
                    return dest_size;
                
                break;
            default:
                chr=chr.offset(fc.width(),0);
                ++sz;
                if(*sz) {
                    gfx::font_char nfc = f[*sz];
                    chr.x2=chr.x1+nfc.width()-1;
                    if(chr.right()>=dest_size.width) {
                        
                        chr.x1=0;
                        chr=chr.offset(0,f.height());
                    }
                    if(chr.x2>=result.width)
                        result.width=chr.x2+1;
                    if(chr.y2>=result.height)
                        result.height=chr.y2+1;    
                    if(chr.y2>dest_size.height)
                        return dest_size;
                    fc=nfc;
                }
                break;
        }
    }
    return result;
}
gfx::ssize16 measure_text2(const gfx::font& f,
    gfx::ssize16 dest_size,
    const char* text,
    unsigned int tab_width) {
    if(nullptr==text || 0==*text)
        return {0,0};
    int mw = 0;
    int w = 0;
    int h = 0;
    int cw;
    const char*sz=text;
    while(*sz) {
        if(h==0) {
            h=f.height();
        }
        gfx::font_char fc = f[*sz];
        switch(*sz) {
            case '\n':
                if(w>mw) {
                    mw = w;
                }
                h+=f.height();
                w=0;
                break;
            case '\r':
                if(w>mw) {
                    mw = w;
                }
                w=0;
                break;
            case '\t':
                cw = f.average_width()*tab_width;
                w=((w/cw)+1)*cw;
                if(w>dest_size.width) {
                    h+=f.height();
                    if(w>mw) {
                        mw = w;
                    }   
                    w=0;
                }
                break;
            default:
                w+=fc.width();
                if(w>dest_size.width) {
                    h+=f.height();
                    w=fc.width();
                }
                if(w>mw) {
                    mw = w;
                }   
                break;
        }
        ++sz;
    }
    return gfx::ssize16(mw,h);
}
void test_font_read() {
    Bm437_Acer_VGA_8x8_FON_stream.seek(0);
    gfx::font f;
    TEST_ASSERT_EQUAL((int)gfx::gfx_result::success,(int)gfx::font::read(&Bm437_Acer_VGA_8x8_FON_stream,&f));
    TEST_ASSERT_EQUAL(8,(int) f.height());
    TEST_ASSERT_EQUAL(8,(int) f.average_width());
}
void test_font_measure_text() {
    const gfx::font& f = Bm437_Acer_VGA_8x8_FON;
    const char* test1 = "test\t1234\r\n5678";
    gfx::ssize16 sz1(32767,32767);
    sz1 = f.measure_text(sz1,test1,4);
    TEST_ASSERT_EQUAL(64+(8*4),(int)sz1.width);
    TEST_ASSERT_EQUAL(16,(int)sz1.height);
    const char* test2 = "test1234";
    gfx::ssize16 sz2(32,32767);
    sz2 = f.measure_text(sz2,test2,4);
    TEST_ASSERT_EQUAL(32,(int)sz2.width);
    TEST_ASSERT_EQUAL(16,(int)sz2.height);
    
}