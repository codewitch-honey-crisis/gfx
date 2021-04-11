#ifndef HTCW_BITS_HPP
#define HTCW_BITS_HPP
#define PACKED __attribute__((__packed__))
#define PACK // TODO: Add Microsoft pack pragmas here
#define RESTORE_PACK

#include <stddef.h>
#include <stdint.h>
#include <string.h>
namespace bits {
    enum struct endian_mode {
        none = 0,
        big_endian = 1,
        little_endian = 2
    };
    // true if the platform is little endian
    constexpr static endian_mode endianness()
    {
#ifdef HTCW_BIG_ENDIAN
        return endian_mode::big_endian;
#endif
#ifdef HTCW_LITTLE_ENDIAN
        return endian_mode::little_endian;
#endif
#ifdef ESP_PLATFORM
        return endian_mode::little_endian;
#else
    return endian_mode::little_endian;    
#endif
        
    }
    // swaps byte order
    constexpr static inline uint16_t swap(uint16_t value)
    {
        return (value >> 8) | (value << 8);
    }
    // swaps byte order
    constexpr static inline uint32_t swap(uint32_t value)
    {
        uint32_t tmp = ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0xFF00FF);
        return (tmp << 16) | (tmp >> 16);
    }
    // swaps byte order
    constexpr static inline uint64_t swap(uint64_t value) {
        value = (value & 0x00000000FFFFFFFF) << 32 | (value & 0xFFFFFFFF00000000) >> 32;
        value = (value & 0x0000FFFF0000FFFF) << 16 | (value & 0xFFFF0000FFFF0000) >> 16;
        value = (value & 0x00FF00FF00FF00FF) << 8  | (value & 0xFF00FF00FF00FF00) >> 8;
        return value;
    }
    // swaps byte order (no-op to resolve ambiguous overload)
    constexpr static inline uint8_t swap(uint8_t value) {
        return value;
    }
    template<size_t SizeBytes>
    constexpr static inline void swap_inline(void* data) {
        switch(SizeBytes) {
            case 0:
            case 1:
                return;
            case 2:
                *((uint16_t*)data)=swap(*(uint16_t*)data);
                return;
            case 4:
                *((uint32_t*)data)=swap(*(uint32_t*)data);
                return;
            case 8:
                *((uint64_t*)data)=swap(*(uint64_t*)data);
                return;
        }
        for (size_t low = 0, high = SizeBytes - 1; low < high; low++, high--) {
            size_t tmp = ((uint8_t*)data)[low];
            ((uint8_t*)data)[low] = ((uint8_t*)data)[high];
            ((uint8_t*)data)[high] = tmp;
        }
    }
    constexpr size_t get_word_size(size_t size) {
        if(size>64) return 0; 
        if(size>32) return 64;
        if(size>16) return 32;
        if(size>8) return 16;
        return 8;
    }

    constexpr size_t get_word_count(size_t size) {
    size_t result = 0;
        while(size>0) {
            size_t ws = get_word_size(size);
            if(ws==0)
            break;
            size-=ws;
            ++result;
        }
        return result;
    }
    constexpr size_t get_left_word(size_t size,size_t index) {
        size_t i = 0;
        while(size>0 && i<=index) {
            size_t ws = get_word_size(size);
            if(ws==0)
            return 0;
            if(index==i)
                return ws;
            size-=ws;
            ++i;
        }
        return 0;
    }
    constexpr size_t get_right_word(size_t size,size_t index) {
        size_t c = get_word_count(size);
        return get_left_word(size,c-index-1);
        size_t i = 0;
        while(size>0 && i<=index) {
            size_t ws = get_word_size(size);
            if(ws==0)
            return 0;
            if(index==i)
                return ws;
            size-=ws;
            ++i;
        }
        return 0;
    }
    template <size_t BitWidth> class uint_helper {};
    template <> struct uint_helper<8> { using type = uint8_t; };
    template <> struct uint_helper<16> { using type = uint16_t; };
    template <> struct uint_helper<32> { using type = uint32_t; };
    template <> struct uint_helper<64> { using type = uint64_t; };
    template<size_t Width> using uintx = typename uint_helper<Width>::type;
    
    template <size_t BitWidth> class real_helper {};
    template <> struct real_helper<32> { using type = float; };
    template <> struct real_helper<64> { using type = double; };
    template<size_t Width> using realx = typename real_helper<Width>::type;
    
    constexpr inline static void set_bits(void* bits,size_t offset_bits,size_t size_bits,bool value) {
        const size_t offset_bytes = offset_bits / 8;
        const size_t offset = offset_bits % 8;
        const size_t total_size_bytes = (offset_bits+size_bits)/8.0+.999999999;
        const size_t overhang = (offset_bits+size_bits) % 8;
        uint8_t* pbegin = ((uint8_t*)bits)+offset_bytes;
        uint8_t* pend = ((uint8_t*)bits)+total_size_bytes;
        uint8_t* plast = pend-(pbegin!=pend);

        const uint8_t maskL = 0!=offset?
                                    (uint8_t)((uint8_t(0xFF>>offset))):
                                    uint8_t(0xff);
        const uint8_t maskR = 0!=overhang?
                                (uint8_t)~((uint8_t(0xFF>>overhang))):
                                uint8_t(0xFF);
        if(value) {
            if(pbegin==plast) {
                const uint8_t mask = maskL & maskR;
                *pbegin|=mask;
                return;
            }
            *pbegin|=maskL;
            *plast|=maskR;
            if(pbegin+1<plast) {
                const size_t len = (plast)-(pbegin+1);
                if(0!=len && len<=total_size_bytes)
                    memset(pbegin+1,0xFF,len);
            }
        } else {
            if(pbegin==plast) {
                const uint8_t mask = maskL & maskR;
                *pbegin&=~mask;
                return;
            }
            *pbegin&=~maskL;
            *plast&=~maskR;
            
            if(pbegin+1<plast) {
                const size_t len = (plast)-(pbegin+1);
                if(0!=len && len<=total_size_bytes)
                    memset(pbegin+1,0,len);
            }
        
        }
    }

    template<size_t OffsetBits,size_t SizeBits,bool Value> constexpr inline static void set_bits(void* bits) {
        if(0==SizeBits)
            return;
        const size_t offset_bytes = OffsetBits / 8;
        const size_t offset = OffsetBits % 8;
        const size_t total_size_bytes = (OffsetBits+SizeBits)/8.0+.999999999;
        const size_t overhang = (OffsetBits+SizeBits) % 8;
        uint8_t* pbegin = ((uint8_t*)bits)+offset_bytes;
        uint8_t* pend = ((uint8_t*)bits)+total_size_bytes;
        uint8_t* plast = pend-(pbegin!=pend);

        const uint8_t maskL = 0!=offset?
                                    (uint8_t)((uint8_t(0xFF>>offset))):
                                    uint8_t(0xff);
        const uint8_t maskR = 0!=overhang?
                                (uint8_t)~((uint8_t(0xFF>>overhang))):
                                uint8_t(0xFF);
        if(Value) {
            if(pbegin==plast) {
                const uint8_t mask = maskL & maskR;
                *pbegin|=mask;
                return;
            }
            *pbegin|=maskL;
            *plast|=maskR;
            if(pbegin+1<plast) {
                const size_t len = (plast)-(pbegin+1);
                if(0!=len && len<=total_size_bytes)
                    memset(pbegin+1,0xFF,len);
            }
        } else {
            if(pbegin==plast) {
                const uint8_t mask = maskL & maskR;
                *pbegin&=~mask;
                return;
            }
            *pbegin&=~maskL;
            *plast&=~maskR;
            
            if(pbegin+1<plast) {
                const size_t len = (plast)-(pbegin+1);
                if(0!=len && len<=total_size_bytes)
                    memset(pbegin+1,0,len);
            }
        
        }
    }
    constexpr inline static void set_bits(size_t offset_bits,size_t size_bits,void* dst,const void* src) {
        const size_t offset_bytes = offset_bits / 8;
        const size_t offset = offset_bits % 8;
        const size_t total_size_bytes = (offset_bits+size_bits)/8.0+.999999999;
        const size_t overhang = (offset_bits+size_bits) % 8;
        uint8_t* pbegin = ((uint8_t*)dst)+offset_bytes;
        uint8_t* psbegin = ((uint8_t*)src)+offset_bytes;
        uint8_t* pend = ((uint8_t*)dst)+total_size_bytes;
        uint8_t* plast = pend-(pbegin!=pend);

        const uint8_t maskL = 0!=offset?
                                    (uint8_t)((uint8_t(0xFF>>offset))):
                                    uint8_t(0xff);
        const uint8_t maskR = 0!=overhang?
                                (uint8_t)~((uint8_t(0xFF>>overhang))):
                                uint8_t(0xFF);
        if(pbegin==plast) {
            uint8_t v = *psbegin;
            const uint8_t mask = maskL & maskR;
            v&=mask;
            *pbegin&=mask;
            *pbegin|=v;
            return;
        }
        *pbegin&=~maskL;
        *pbegin|=((*psbegin)&maskL);
        *plast&=~maskR;
        *plast|=((*(psbegin+total_size_bytes-1))&maskR);
        if(pbegin+1<plast) {
            const size_t len = plast-(pbegin+1);
            if(0!=len&&len<=total_size_bytes) 
                memcpy(pbegin+1,psbegin+1,len);
        }
    }
    template<size_t OffsetBits,size_t SizeBits> constexpr inline static void set_bits(void* dst,const void* src) {
        const size_t offset_bytes = OffsetBits / 8;
        const size_t offset = OffsetBits % 8;
        const size_t total_size_bytes = (OffsetBits+SizeBits)/8.0+.999999999;
        const size_t overhang = (OffsetBits+SizeBits) % 8;
        uint8_t* pbegin = ((uint8_t*)dst)+offset_bytes;
        uint8_t* psbegin = ((uint8_t*)src)+offset_bytes;
        uint8_t* pend = ((uint8_t*)dst)+total_size_bytes;
        uint8_t* plast = pend-(pbegin!=pend);
        uint8_t* pslast =psbegin+total_size_bytes-1;
        const uint8_t maskL = 0!=offset?
                                    (uint8_t)((uint8_t(0xFF>>offset))):
                                    uint8_t(0xff);
        const uint8_t maskR = 0!=overhang?
                                (uint8_t)~((uint8_t(0xFF>>overhang))):
                                uint8_t(0xFF);
        if(pbegin==plast) {
            uint8_t v = *psbegin;
            const uint8_t mask = maskL & maskR;
            v&=mask;
            *pbegin=(*pbegin&uint8_t(~mask))|v;
            return;
        }
        *pbegin=(*pbegin&~maskL)|((*psbegin)&maskL);
        
        *plast=(*plast&~maskR)|((*pslast)&maskR);
        if(pbegin+1<plast) {
            const size_t len = plast-(pbegin+1);
            if(0!=len&&len<=total_size_bytes) 
                memcpy(pbegin+1,psbegin+1,len);
        }
    }
    
    template<size_t OffsetBits,size_t SizeBits, size_t Shift> constexpr inline static void shift_left(void* bits) {
        if(nullptr==bits || 0==SizeBits || 0==Shift) {
            return;
        }
        // special case if we shift all the bits out
        if(Shift>=SizeBits) {
            set_bits<OffsetBits,SizeBits,false>(bits);
            return;
        }
        uint8_t* pbegin = ((uint8_t*)bits)+(OffsetBits/8);
        const size_t offset = OffsetBits % 8;
        const size_t shift_bytes = Shift / 8;
        const size_t shift_bits = Shift % 8;
        const size_t overhang = (SizeBits+OffsetBits) % 8;
        // preserves left prior to offset
        const uint8_t left_mask = ((uint8_t)uint8_t(0xFF<<(8-offset)));
        // preserves right after overhang
        const uint8_t right_mask = 0!=overhang?uint8_t(0xFF>>overhang):0;
        uint8_t* pend = pbegin+(size_t)((OffsetBits+SizeBits)/8.0+.999999);
        uint8_t* plast = pend-1;
        uint8_t* psrc = pbegin+shift_bytes;
        uint8_t* pdst = pbegin;
        if(pbegin+1==pend) {
            // special case for a shift all within one byte
            uint8_t save_mask = left_mask|right_mask;
            uint8_t tmp = *pbegin;
            *pbegin = uint8_t(uint8_t(tmp<<shift_bits)&~save_mask)|
                    uint8_t(tmp&save_mask);
            return;
        }
        // preserve the ends so we can
        // fix them up later
        uint8_t left = *pbegin;
        uint8_t right = *(pend-1);
        
        while(pdst!=pend) {
            uint8_t src = psrc<pend?*psrc:0;
            uint8_t src2 = (psrc+1)<pend?*(psrc+1):0;
            *pdst = (src<<shift_bits)|(src2>>(8-shift_bits));
            ++psrc;
            ++pdst;
        }
        
        *pbegin=(left&left_mask)|uint8_t(*pbegin&~left_mask);
        --pend;
        *plast=uint8_t(right&right_mask)|uint8_t(*plast&uint8_t(~right_mask));
    };

    constexpr static void shift_left(void* bits,size_t offset_bits,size_t size_bits, size_t shift) {
        if(nullptr==bits || 0==size_bits || 0==shift) {
            return;
        }
        // special case if we shift all the bits out
        if(shift>=size_bits) {
            set_bits(bits,offset_bits,size_bits,false);
            return;
        }
        uint8_t* pbegin = ((uint8_t*)bits)+(offset_bits/8);
        const size_t offset = offset_bits % 8;
        const size_t shift_bytes = shift / 8;
        const size_t shift_bits = shift % 8;
        const size_t overhang = (size_bits+offset_bits) % 8;
        // preserves left prior to offset
        const uint8_t left_mask = ((uint8_t)uint8_t(0xFF<<(8-offset)));
        // preserves right after overhang
        const uint8_t right_mask = 0!=overhang?uint8_t(0xFF>>overhang):0;
        uint8_t* pend = pbegin+(size_t)((offset_bits+size_bits)/8.0+.999999);
        uint8_t* plast = pend-1;
        uint8_t* psrc = pbegin+shift_bytes;
        uint8_t* pdst = pbegin;
        if(pbegin+1==pend) {
            // special case for a shift all within one byte
            uint8_t save_mask = left_mask|right_mask;
            uint8_t tmp = *pbegin;
            *pbegin = uint8_t(uint8_t(tmp<<shift_bits)&~save_mask)|
                    uint8_t(tmp&save_mask);
            return;
        }
        // preserve the ends so we can
        // fix them up later
        uint8_t left = *pbegin;
        uint8_t right = *(pend-1);
        
        while(pdst!=pend) {
            uint8_t src = psrc<pend?*psrc:0;
            uint8_t src2 = (psrc+1)<pend?*(psrc+1):0;
            *pdst = (src<<shift_bits)|(src2>>(8-shift_bits));
            ++psrc;
            ++pdst;
        }
        
        *pbegin=(left&left_mask)|uint8_t(*pbegin&~left_mask);
        --pend;
        *plast=uint8_t(right&right_mask)|uint8_t(*plast&uint8_t(~right_mask));
    };

    template<size_t OffsetBits,size_t SizeBits, size_t Shift> constexpr inline static void shift_right(void* bits) {
        if(nullptr==bits || 0==SizeBits || 0==Shift) {
            return;
        }
        // special case if we shift all the bits out
        if(Shift>=SizeBits) {
            set_bits<OffsetBits,SizeBits,false>(bits);
            return;
        }
        uint8_t* pbegin = ((uint8_t*)bits)+(OffsetBits/8);
        const size_t offset = OffsetBits % 8;
        const size_t shift_bytes = Shift / 8;
        const size_t shift_bits = Shift % 8;
        const size_t overhang = (SizeBits+OffsetBits) % 8;
        // preserves left prior to offset
        const uint8_t left_mask = ((uint8_t)uint8_t(0xFF<<(8-offset)));
        // preserves right after overhang
        const uint8_t right_mask = 0!=overhang?uint8_t(0xFF>>overhang):0;
        uint8_t* pend = pbegin+(size_t)((OffsetBits+SizeBits)/8.0+.999999)-(OffsetBits/8);
        uint8_t* plast = pend-1;
        uint8_t* psrc = (pend-1)-shift_bytes;
        uint8_t* pdst = pend-1;
        if(pbegin+1==pend) {
            // special case for a shift all within one byte
            uint8_t save_mask = left_mask|right_mask;
            uint8_t tmp = *pbegin;
            *pbegin = uint8_t(uint8_t(tmp>>shift_bits)&~save_mask)|
                    uint8_t(tmp&save_mask);
            return;
        }
        // preserve the ends so we can
        // fix them up later
        uint8_t left = *pbegin;
        uint8_t right = *psrc;
        
        while(pdst>=pbegin) {
            uint8_t src = psrc>=pbegin?*psrc:0;
            uint8_t src2 = (psrc-1)>=pbegin?*(psrc-1):0;
            *pdst = (src>>shift_bits)|(src2<<(8-shift_bits));
            --psrc;
            --pdst;
        }
        
        *pbegin=uint8_t(left&left_mask)|uint8_t(*pbegin&~left_mask);
        *plast=uint8_t(right&right_mask)|uint8_t(*plast&uint8_t(~right_mask));
    };
    constexpr static void shift_right(void* bits,size_t offset_bits,size_t size_bits, size_t shift) {
        if(nullptr==bits || 0==size_bits || 0==shift) {
            return;
        }
        // special case if we shift all the bits out
        if(shift>=size_bits) {
            set_bits(bits,offset_bits,size_bits,false);
            return;
        }
        uint8_t* pbegin = ((uint8_t*)bits)+(offset_bits/8);
        const size_t offset = offset_bits % 8;
        const size_t shift_bytes = shift / 8;
        const size_t shift_bits = shift % 8;
        const size_t overhang = (size_bits+offset_bits) % 8;
        // preserves left prior to offset
        const uint8_t left_mask = ((uint8_t)uint8_t(0xFF<<(8-offset)));
        // preserves right after overhang
        const uint8_t right_mask = 0!=overhang?uint8_t(0xFF>>overhang):0;
        uint8_t* pend = pbegin+(size_t)((offset_bits+size_bits)/8.0+.999999)-(offset_bits/8);
        uint8_t* plast = pend-1;
        uint8_t* psrc = (pend-1)-shift_bytes;
        uint8_t* pdst = pend-1;
        if(pbegin+1==pend) {
            // special case for a shift all within one byte
            uint8_t save_mask = left_mask|right_mask;
            uint8_t tmp = *pbegin;
            *pbegin = uint8_t(uint8_t(tmp>>shift_bits)&~save_mask)|
                    uint8_t(tmp&save_mask);
            return;
        }
        // preserve the ends so we can
        // fix them up later
        uint8_t left = *pbegin;
        uint8_t right = *psrc;
        
        while(pdst>=pbegin) {
            uint8_t src = psrc>=pbegin?*psrc:0;
            uint8_t src2 = (psrc-1)>=pbegin?*(psrc-1):0;
            *pdst = (src>>shift_bits)|(src2<<(8-shift_bits));
            --psrc;
            --pdst;
        }
        
        *pbegin=uint8_t(left&left_mask)|uint8_t(*pbegin&~left_mask);
        *plast=uint8_t(right&right_mask)|uint8_t(*plast&uint8_t(~right_mask));
    };
}
#endif