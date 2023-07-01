#ifndef HTCW_GFX_IMAGE_HPP
#define HTCW_GFX_IMAGE_HPP
#include "gfx_core.hpp"
#include "gfx_pixel.hpp"
#include "gfx_palette.hpp"
#include "gfx_positioning.hpp"
#include "gfx_bitmap.hpp"

//#define GFX_JPEG_AS_RGB
// Jpg decompression code ported from TJpgDec. The original
// copyright notice is below:
/*----------------------------------------------------------------------------/
/ TJpgDec - Tiny JPEG Decompressor include file               (C)ChaN, 2020
/----------------------------------------------------------------------------*/

namespace gfx {
    struct png_image final {
        using pixel_type = rgba_pixel<32>;
        typedef gfx_result(*callback)(size16 dimensions, const rect16& bounds,pixel_type color, void* state);
        static gfx_result dimensions(stream* input, size16* out_dimensions);
        static gfx_result load(stream* input,callback out_func,void* state=nullptr,uint8_t*fourcc=nullptr);
#ifdef ARDUINO
        inline static gfx_result load(Stream* input,callback out_func,void* state=nullptr,uint8_t* fourcc=nullptr) {
            arduino_stream stm(input);
            return load(&stm,out_func,state,fourcc);
        }
#endif  
    private:

    };
    struct jpeg_image final {
        #ifdef GFX_JPEG_AS_RGB
        using pixel_type = rgb_pixel<24>;
        #else
        using pixel_type = ycbcr_pixel<24>;
        #endif
        using region_type = bitmap<pixel_type,palette<pixel_type,pixel_type>>;
        // region is not const so we can do in place filtering
        typedef gfx_result(*callback)(size16 dimensions, region_type& region,point16 location,void* state);
    private:
        // this private section is all ported from tjpgd.c
        // TJpgDec - Tiny JPEG Decompressor include file               (C)ChaN, 2020
    	/* Error code */
        enum JRESULT
        {
            JDR_OK = 0, /* 0: Succeeded */
            JDR_INTR,	/* 1: Interrupted by output function */
            JDR_INP,	/* 2: Device error or wrong termination of input stream */
            JDR_MEM1,	/* 3: Insufficient memory pool for the image */
            JDR_MEM2,	/* 4: Insufficient stream input buffer */
            JDR_PAR,	/* 5: Parameter error */
            JDR_FMT1,	/* 6: Data format error (may be damaged data) */
            JDR_FMT2,	/* 7: Right format but not supported */
            JDR_FMT3	/* 8: Not supported JPEG standard */
        };
        struct JRECT final 
        {
            uint16_t left, right, top, bottom;
        } ;

        struct JDEC final
        {
            uint8_t fourcc[4];
            uint8_t read_fourcc;
            unsigned int dctr;										 /* Number of bytes available in the input buffer */
            uint8_t *dptr;											 /* Current data read ptr */
            uint8_t *inbuf;											 /* Bit stream input buffer */
            uint8_t dmsk;											 /* Current bit in the current read byte */
            uint8_t scale;											 /* Output scaling ratio */
            uint8_t msx, msy;										 /* MCU size in unit of block (width, height) */
            uint8_t qtid[3];										 /* Quantization table ID of each component */
            int16_t dcv[3];											 /* Previous DC element of each component */
            uint16_t nrst;											 /* Restart inverval */
            uint16_t width, height;									 /* Size of the input image (pixel) */
            uint8_t *huffbits[2][2];								 /* Huffman bit distribution tables [id][dcac] */
            uint16_t *huffcode[2][2];								 /* Huffman code word tables [id][dcac] */
            uint8_t *huffdata[2][2];								 /* Huffman decoded data tables [id][dcac] */
            int32_t *qttbl[4];										 /* Dequantizer tables [id] */
            void *workbuf;											 /* Working buffer for IDCT and RGB output */
            uint8_t *mcubuf;										 /* Working buffer for the MCU */
            void *pool;												 /* Pointer to available memory pool */
            unsigned int sz_pool;									 /* Size of momory pool (bytes available) */
            unsigned int (*infunc)(JDEC *, uint8_t *, unsigned int); /* Pointer to jpeg stream input function */
            void *device;											 /* Pointer to I/O device identifiler for the session */
        };
        static const uint8_t* Zig;
        static const uint16_t* Ipsf;
        #ifdef JD_TBLCLIP
        static const uint8_t* Clip8;
        static inline uint8_t BYTECLIP(int val) {
            return Clip8[(unsigned int)(val)&0x3FF];
        }
        #else 
        static inline uint8_t BYTECLIP(int val)
        {
            return (uint8_t)helpers::clamp(val,0,255);
        }
        #endif
        static inline uint8_t ZIG(int n) {
            return Zig[n];
        }
        static inline uint16_t IPSF(int n) {
            return Ipsf[n];
        }
        constexpr static const int WORKSZ =3100; // size of workspace for JPEG decoder
        constexpr static const int JD_SZBUF= 512;   /* Size of stream input buffer */
        constexpr static const int JD_TBLCLIP= 1;   /* Use table for saturation (might be a bit faster but increases 1K bytes of code size) */

        static void *alloc_pool(				/* Pointer to allocated memory block (NULL:no memory available) */
							JDEC *jd,		/* Pointer to the decompressor object */
							unsigned int nd /* Number of bytes to allocate */
        );
    	static JRESULT create_qt_tbl(					  /* 0:OK, !0:Failed */
								 JDEC *jd,			  /* Pointer to the decompressor object */
								 const uint8_t *data, /* Pointer to the quantizer tables */
								 unsigned int ndata	  /* Size of input data */
        );
        
       	static JRESULT create_huffman_tbl(					   /* 0:OK, !0:Failed */
                                    JDEC *jd,			   /* Pointer to the decompressor object */
                                    const uint8_t *data, /* Pointer to the packed huffman tables */
                                    unsigned int ndata   /* Size of input data */
        );
        static int bitext(				/* >=0: extracted data, <0: error code */
					  JDEC *jd,			/* Pointer to the decompressor object */
					  unsigned int nbit /* Number of bits to extract (1 to 11) */
        );
        static int huffext(					  /* >=0: decoded data, <0: error code */
					   JDEC *jd,			  /* Pointer to the decompressor object */
					   const uint8_t *hbits,  /* Pointer to the bit distribution table */
					   const uint16_t *hcode, /* Pointer to the code word table */
					   const uint8_t *hdata	  /* Pointer to the data table */
        );
        static void block_idct(
            int32_t *src, /* Input block data (de-quantized and pre-scaled for Arai Algorithm) */
            uint8_t *dst  /* Pointer to the destination to store the block as byte array */
        );
        static JRESULT mcu_load(
            JDEC *jd /* Pointer to the decompressor object */
        );
        static JRESULT mcu_output(
            JDEC *jd,								 /* Pointer to the decompressor object */
            int (*outfunc)(JDEC *, void *, JRECT *), /* RGB output function */
            unsigned int x,							 /* MCU position in the image (left of the MCU) */
            unsigned int y							 /* MCU position in the image (top of the MCU) */
        );
        static JRESULT restart(
            JDEC *jd,	  /* Pointer to the decompressor object */
            uint16_t rstn /* Expected restert sequense number */
        );
        static uint16_t LDB_WORD(const uint8_t* ptr);
        static JRESULT jd_prepare(
            JDEC *jd,												 /* Blank decompressor object */
            unsigned int (*infunc)(JDEC *, uint8_t *, unsigned int), /* JPEG strem input function */
            void *pool,												 /* Working buffer for the decompression session */
            unsigned int sz_pool,									 /* Size of working buffer */
            void *dev												 /* I/O device identifier for the session */
        );
        static JRESULT jd_decomp(
            JDEC *jd,								 /* Initialized decompression object */
            int (*outfunc)(JDEC *, void *, JRECT *), /* RGB output function */
            uint8_t scale							 /* Output de-scaling factor (0 to 3) */
        );
        // the following are bindings for GFX to tjpgd:
        typedef struct {
            stream* input_stream;
            callback out;
            void* state;
            gfx_result result;
        } JpegDev;
        static gfx_result xlt_err(int jderr);
        //Input function for jpeg decoder. Just returns bytes from the inData field of the JpegDev structure.
        static unsigned int infunc(JDEC *decoder, uint8_t *buf, unsigned int len);

        static int outfunc(JDEC *decoder, void *bitmap, JRECT *rect);
    public:
        static gfx_result dimensions(stream* input, size16* out_dimensions);
        static gfx_result load(stream* input,callback out_func,void* state=nullptr,uint8_t*fourcc=nullptr);
#ifdef ARDUINO
        inline static gfx_result load(Stream* input,callback out_func,void* state=nullptr,uint8_t* fourcc=nullptr) {
            arduino_stream stm(input);
            return load(&stm,out_func,state,fourcc);
        }
#endif  
    };
    
}
#endif