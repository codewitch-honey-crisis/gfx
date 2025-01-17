#include "gfx_jpg_image.hpp"
#include "tjpgd.h"
namespace gfx {
#define LDB_WORD(ptr) (uint16_t)(((uint16_t) * ((uint8_t *)(ptr)) << 8) | (uint16_t) * (uint8_t *)((ptr) + 1))
static uint8_t jpg_image_buffer[JD_SZBUF];

jpg_image::jpg_image() : m_stream(nullptr), m_native_dimensions(0, 0), m_scale(jpg_scale::scale_1_1), m_info(nullptr), m_pool(nullptr),m_bmp(nullptr)
{
}
jpg_image::jpg_image(stream &stream, jpg_scale scale, bool initialize) : m_stream(&stream),m_native_dimensions(0,0), m_scale(scale), m_info(nullptr), m_pool(nullptr),m_bmp(nullptr)
{
    if (initialize)
    {
        this->initialize();
    }
}
jpg_image::jpg_image(jpg_image &&rhs) : m_stream(rhs.m_stream), m_native_dimensions(rhs.m_native_dimensions),m_scale(rhs.m_scale), m_info(rhs.m_info), m_pool(rhs.m_pool),m_bmp(rhs.m_bmp)
{
    rhs.m_stream = nullptr;
    rhs.m_native_dimensions = {0, 0};
    rhs.m_info = nullptr;
    rhs.m_pool = nullptr;
    rhs.m_bmp = nullptr;
}
jpg_image &jpg_image::operator=(jpg_image &&rhs)
{
    this->deinitialize();
    m_stream = rhs.m_stream;
    m_native_dimensions=rhs.m_native_dimensions;
    m_scale = rhs.m_scale;
    m_info = rhs.m_info;
    m_pool = rhs.m_pool;
    m_bmp = rhs.m_bmp;
    rhs.m_stream = nullptr;
    rhs.m_native_dimensions = {0, 0};
    rhs.m_info = nullptr;
    rhs.m_pool = nullptr;
    rhs.m_bmp = nullptr;
    return *this;
}
jpg_image::~jpg_image()
{
    this->deinitialize();
}
gfx_result jpg_image::initialize()
{
    if (initialized())
    {
        return gfx_result::success;
    }
    if (m_stream == nullptr)
    {
        return gfx_result::invalid_state;
    }
    if (m_stream->caps().read == 0 || m_stream->caps().seek == 0)
    {
        return gfx_result::invalid_argument;
    }
    m_stream->seek(0);
    
    unsigned int ofs;
    uint16_t marker;
    ofs = marker = 0;
    uint8_t tmp8;
    uint8_t data4[4];
    size_t len;
    //int div;
    unsigned long long pos;
    do
    {
        if (1 != m_stream->read(&tmp8, 1))
        {
            return gfx_result::io_error;
        }
        ++ofs;
        marker = marker << 8 | tmp8;
    } while (marker != 0xFFD8);
    while (1)
    {
        if (4 != m_stream->read(data4, 4))
        {
            return gfx_result::io_error;
        }
        marker = LDB_WORD(data4);  /* Marker */
        len = LDB_WORD(data4 + 2); /* Length field */
        if (len <= 2 || (marker >> 8) != 0xFF)
            return gfx_result::invalid_format;
        len -= 2;       /* Segent content size */
        ofs += 4 + len; /* Number of bytes loaded */
        switch (marker & 0xFF)
        {
        case 0xC0: /* SOF0 (baseline JPEG) */
            if (len > JD_SZBUF)
                return gfx_result::out_of_memory;
            if (len != m_stream->read(jpg_image_buffer, len))
            {
                return gfx_result::io_error;
            }
            m_native_dimensions.width = LDB_WORD(&jpg_image_buffer[3]);
            m_native_dimensions.height = LDB_WORD(&jpg_image_buffer[1]);
            m_stream->seek(0);
            m_info = malloc(sizeof(JDEC));
            if (m_info == nullptr)
            {
                return gfx_result::out_of_memory;
            }
            m_pool = malloc(pool_size);
            if (m_pool == nullptr)
            {
                free(m_info);
                m_info = nullptr;
                return gfx_result::out_of_memory;
            }
            ((JDEC *)m_info)->device = nullptr;
            m_bmp = malloc(bitmap<rgba_pixel<32>>::sizeof_buffer({16,16}));
            if(m_bmp==nullptr) {
                free(m_pool);
                m_pool=nullptr;
                free(m_info);
                m_info = nullptr;
                return gfx_result::out_of_memory;
            }
            return gfx_result::success;
        default:
            pos = m_stream->seek(0, seek_origin::current);
            if (pos + len != m_stream->seek(len, seek_origin::current))
            {
                return gfx_result::invalid_format;
            }

            break;
        }
    }

    return gfx_result::unknown_error;
}
bool jpg_image::initialized() const
{
    return m_native_dimensions.width != 0 && m_native_dimensions.height != 0;
}
void jpg_image::deinitialize()
{
    if (m_info)
    {
        free(m_info);
        m_info = nullptr;
    }
    if (m_pool)
    {
        free(m_pool);
        m_pool = nullptr;
    }
    if(m_bmp) {
        free(m_bmp);
        m_bmp = nullptr;
    }
    m_native_dimensions = {0, 0};
}
size16 jpg_image::dimensions() const
{
    int div = powf(2, (int)m_scale);
    return size16(m_native_dimensions.width/div,m_native_dimensions.height/div);
}
size16 jpg_image::native_dimensions() const
{
    return m_native_dimensions;
}
jpg_scale jpg_image::scale() const {
    return m_scale;
}
void jpg_image::scale(jpg_scale value) {
    
    m_scale = value;
}
gfx_result jpg_image::draw(const rect16 &bounds, image_draw_callback callback, void *callback_state) const
{
    if (!initialized())
        return gfx_result::invalid_state;
    if (callback == nullptr)
        return gfx_result::invalid_argument;
    JDEC *jdec = (JDEC *)m_info;
    struct dec_state {
        stream* stm;
        void*bmp;
        const rect16* bounds;
        image_draw_callback cb;
        void* cb_state;
        gfx_result error;
    };
    dec_state st;
    st.stm = m_stream;
    if(m_stream->caps().seek) {
        m_stream->seek(0);
    }
    st.bounds = &bounds;
    st.bmp = m_bmp;
    st.cb = callback;
    st.error = gfx_result::success;
    st.cb_state = callback_state;
    JRESULT res = jd_prepare(jdec, [](JDEC *jdec,    /* Pointer to the decompression object */
                                      uint8_t *buff, /* Pointer to buffer to store the read data */
                                      size_t ndata   /* Number of bytes to read/remove */
                                   )
                             { 
                                size_t result;
                                if(buff==nullptr) {
                                    unsigned long long pos = ((dec_state *)jdec->device)->stm->seek(0,seek_origin::current);
                                    result = ((dec_state *)jdec->device)->stm->seek(ndata,seek_origin::current)-pos;
                                } else {
                                    result = (((dec_state *)jdec->device)->stm->read(buff, ndata));;
                                }
                                return result;
                             },
                             m_pool, pool_size, &st);
    if (res != JDR_OK)
    {
        switch (res)
        {
        case JDR_INP:
            return gfx_result::io_error;
        case JDR_MEM1:
            return gfx_result::out_of_memory;
        case JDR_MEM2:
            return gfx_result::out_of_memory;
        case JDR_FMT1:
            return gfx_result::invalid_format;
        case JDR_FMT2:
            return gfx_result::not_supported;
        case JDR_FMT3:
            return gfx_result::not_supported;
        case JDR_INTR:
            return st.error;
        case JDR_PAR:
            return gfx_result::invalid_argument;
        default:
            break;
        }
        return gfx_result::unknown_error;
    }
    res = jd_decomp(jdec, [](JDEC *jdec,   /* Pointer to the decompression object */
                             void *bmp, /* Bitmap to be output */
                             JRECT *rect   /* Rectangle to output */
                          )
                    {
                        dec_state& st=*(dec_state*)jdec->device;
                        int x1 = rect->left;
                        int y1 = rect->top;
                        int x2 = rect->right;
                        int y2 = rect->bottom;
                        int w = x2-x1+1;
                        int h = y2-y1+1;
                        if(w*h>256) {
                            // region was bigger than we prepared for
                            st.error = gfx_result::out_of_memory;
                            return 0;
                        }
                        // we're within the destination
                        if((x2>=st.bounds->x1 && x1<=st.bounds->x2) && 
                            (y2>=st.bounds->y1 && y1<=st.bounds->y2)) {
                            const int offsx = -st.bounds->x1;
                            const int offsy = -st.bounds->y1;
                            const bool left_edge = x2>=st.bounds->x1 && x1<st.bounds->x1;
                            const bool top_edge = y2>=st.bounds->y1 && y1<st.bounds->y1;
                            int xs=0,xc=w;
                            if(left_edge) {
                                xs=st.bounds->x1-x1;
                                xc = w-xs;
                            }
                            int ys=0,yc=h;
                            if(top_edge) {
                                ys=st.bounds->y1-y1;
                                yc = h-ys;
                            }
                            const uint8_t* pbs = (const uint8_t*)bmp;
                            uint8_t* pbd = ( uint8_t*)st.bmp;
                            for(int y=0;y<yc;++y) {
                                const uint8_t* ps = pbs+(((y+ys)*w*3)+(xs*3));
                                uint8_t* pd = pbd+(y*xc*4);
                                for(int x=0;x<xc;++x) {
                                    *pd++=*ps++;
                                    *pd++=*ps++;
                                    *pd++=*ps++;
                                    *pd++=0xFF;
                                }
                            }
                            image_data data;
                            data.is_fill = false;
                            const const_bitmap<rgba_pixel<32>> csrc(size16(xc,yc),st.bmp);
                            data.bitmap.region = &csrc;
                            data.bitmap.location = point16(x1+offsx,y1+offsy);
                            gfx_result r =st.cb(data,st.cb_state);
                            if(gfx_result::success!=r) {
                                st.error = r;
                                return 0;
                            }
                             
                        } else {
                            if(y1>st.bounds->y2) {
                                st.error = gfx_result::success;
                                // don't need to display any more...
                                return 0;
                            } 
                        }
                        
                        return (int)1; 
                    },
                    (uint8_t)(int)m_scale);
    if (res != JDR_OK)
    {
        switch (res)
        {
        case JDR_INP:
            return gfx_result::io_error;
        case JDR_PAR:
            return gfx_result::invalid_argument;
        case JDR_MEM1:
            return gfx_result::out_of_memory;
        case JDR_MEM2:
            return gfx_result::out_of_memory;
        case JDR_FMT1:
            return gfx_result::invalid_format;
        case JDR_FMT2:
            return gfx_result::not_supported;
        case JDR_FMT3:
            return gfx_result::not_supported;
        case JDR_INTR:
            return st.error;
        default:
            break;
        }
        return gfx_result::unknown_error;
    }
    return gfx_result::success;
}
}