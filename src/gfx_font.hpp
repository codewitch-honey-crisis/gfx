#ifndef HTCW_GFX_FONT_HPP
#define HTCW_GFX_FONT_HPP
#include <gfx_core.hpp>
#include <gfx_positioning.hpp>
#include <gfx_pixel.hpp>
#include <gfx_palette.hpp>
#include <gfx_bitmap.hpp>
#include <gfx_encoding.hpp>
#include <htcw_data.hpp>
namespace gfx {
    struct font_glyph_info final {
        size16 dimensions;
        int16_t advance_width;
        spoint16 offset;
        int glyph_index1;
        int glyph_index2;
    };
    enum struct font_size_units {
        em = 0,
        px = 1
    };
    typedef gfx_result(*font_draw_callback)(spoint16 location,const const_bitmap<alpha_pixel<8>>& glyph_icon, void* state);
    class font_draw_cache {
        typedef struct {
            int accessed;
            size16 dimensions;
            uint8_t* data;
        } cache_entry_t;
        using map_t = data::simple_fixed_map<int32_t,cache_entry_t,32>;
        void*(*m_allocator)(size_t);
        void*(*m_reallocator)(void*,size_t);
        void(*m_deallocator)(void*);
        bool m_initialized;
        int m_accessed;
        map_t m_cache;
        size_t m_memory_size;
        size_t m_max_memory_size;
        size_t m_max_entries;
        font_draw_cache(const font_draw_cache& rhs)=delete;
        font_draw_cache& operator=(const font_draw_cache& rhs)=delete;
        static int hash_function(const int32_t& key); 
        void expire_memory(size_t new_data_size);
        void expire_item();
        void reduce(int new_size, int new_item_size);
    public:
        font_draw_cache(void*(allocator)(size_t)=::malloc, void*(reallocator)(void*,size_t)=::realloc, void(deallocator)(void*)=::free);
        font_draw_cache(font_draw_cache&& rhs);
        virtual ~font_draw_cache();
        font_draw_cache& operator=(font_draw_cache&& rhs);
        size_t max_memory_size() const;
        void max_memory_size(size_t value);
        size_t memory_size() const;
        size_t max_entries() const;
        void max_entries(size_t value);
        size_t entries() const;
        gfx_result add(int32_t codepoint, size16 dimensions, const uint8_t* data);
        gfx_result find(int32_t codepoint, size16* out_dimensions, uint8_t ** out_bitmap);
        void clear();
        gfx_result initialize();
        bool initialized() const;
        void deinitialize();
    };
    class font_measure_cache {
        typedef struct measure_key {
            uint8_t value[8];
            inline bool operator==(const measure_key& key) const {
                return 0==memcmp(key.value,value,8);
            }
        } key_t;
        typedef struct {
            int accessed;
            font_glyph_info data;
        } cache_entry_t;
        using map_t = data::simple_fixed_map<key_t,cache_entry_t,64>;
        void*(*m_allocator)(size_t);
        void*(*m_reallocator)(void*,size_t);
        void(*m_deallocator)(void*);
        bool m_initialized;
        int m_accessed;
        map_t m_cache;
        size_t m_memory_size;
        size_t m_max_memory_size;
        size_t m_max_entries;
        font_measure_cache(const font_measure_cache& rhs)=delete;
        font_measure_cache& operator=(const font_measure_cache& rhs)=delete;
        static int hash_function(const key_t& key); 
        static void make_key(int32_t codepoint1, int32_t codepoint2, key_t* out_key);
        void expire_memory(size_t new_data_size);
        void expire_item();
        void reduce(int new_size, int new_item_size);
    public:
        font_measure_cache(void*(allocator)(size_t)=::malloc, void*(reallocator)(void*,size_t)=::realloc, void(deallocator)(void*)=::free);
        font_measure_cache(font_measure_cache&& rhs);
        virtual ~font_measure_cache();
        font_measure_cache& operator=(font_measure_cache&& rhs);
        size_t max_memory_size() const;
        void max_memory_size(size_t value);
        size_t memory_size() const;
        size_t max_entries() const;
        void max_entries(size_t value);
        size_t entries() const;
        gfx_result add(int32_t codepoint1, int32_t codepoint2, const font_glyph_info& data);
        gfx_result find(int32_t codepoint1, int32_t codepoint2, font_glyph_info* out_glyph_info);
        void clear();
        gfx_result initialize();
        bool initialized() const;
        void deinitialize();
    };
    class font;
    struct text_info final {
        text_handle text;
        size_t text_byte_count;
        const font* text_font;
        unsigned int tab_width;
        const text_encoder* encoding;
        font_measure_cache* measure_cache;
        font_draw_cache* draw_cache;
        inline text_info() : text_font(nullptr) {
            text = nullptr;
            text_byte_count = 0;
            tab_width = 4;
            encoding = &text_encoding::utf8;
            measure_cache = nullptr;
            draw_cache = nullptr;
        }
        inline text_info(const text_handle text, size_t text_byte_count, const ::gfx::font& font, int tab_width = 4, const text_encoder& encoding = text_encoding::utf8,font_measure_cache* measure_cache = nullptr, font_draw_cache* draw_cache =nullptr ) {
            this->text = text;
            this->text_byte_count = text_byte_count;
            this->text_font = &font;
            this->tab_width = tab_width;
            this->encoding = &encoding;
            this->measure_cache = measure_cache;
            this->draw_cache = draw_cache;
        }
        inline text_info(const char* text, const ::gfx::font& font, int tab_width = 4, const text_encoder& encoding = text_encoding::utf8,font_measure_cache* measure_cache = nullptr, font_draw_cache* draw_cache =nullptr ) {
            this->text = (text_handle)text;
            this->text_byte_count = strlen(text);
            this->text_byte_count = text_byte_count;
            this->text_font = &font;
            this->tab_width = tab_width;
            this->encoding = &encoding;
            this->measure_cache = measure_cache;
            this->draw_cache = draw_cache;
        }
        inline void text_sz(const char* text) {
            this->text = (text_handle)text;
            if(text!=nullptr) {
                this->text_byte_count=strlen(text);
            } else {
                this->text_byte_count = 0;
            }
        }
    };
    class font {
    protected:
        virtual gfx_result on_measure(int32_t codepoint1,int32_t codepoint2, font_glyph_info* out_glyph_info) const=0;
        virtual gfx_result on_draw(bitmap<alpha_pixel<8>>& destination,int32_t codepoint, int32_t glyph_index = -1) const=0;
    public:
        virtual gfx_result initialize()=0;
        virtual bool initialized() const=0;
        virtual void deinitialize()=0;
        virtual uint16_t line_height() const = 0;
        virtual uint16_t line_advance() const = 0;
        virtual uint16_t base_line() const = 0;
    public:
        gfx_result measure(uint16_t max_width,const text_handle text, size_t text_data_len, size16* out_area, uint16_t tab_width = 4, const text_encoder& encoding = text_encoding::utf8, font_measure_cache* cache = nullptr) const;
        gfx_result draw(const srect16& bounds, const text_handle text,size_t text_data_len, font_draw_callback callback, void* callback_state=nullptr,  uint16_t tab_width = 4, const text_encoder& encoding = text_encoding::utf8, font_draw_cache* draw_cache = nullptr, font_measure_cache* measure_cache = nullptr) const;
        inline gfx_result measure(uint16_t max_width,const char* text, size16* out_area, uint16_t tab_width = 4, const text_encoder& encoding = text_encoding::utf8, font_measure_cache* cache = nullptr) const {
            return this->measure(max_width,(text_handle)text,strlen(text),out_area,tab_width,encoding,cache);
        }
        inline gfx_result measure(uint16_t max_width,const text_info& ti, size16* out_area) const {
            return this->measure(max_width,ti.text,ti.text_byte_count,out_area, ti.tab_width,*ti.encoding,ti.measure_cache);
        }
        inline gfx_result draw(const srect16& bounds, const char* text, font_draw_callback callback, void* callback_state=nullptr,  uint16_t tab_width = 4, const text_encoder& encoding = text_encoding::utf8, font_draw_cache* draw_cache = nullptr, font_measure_cache* measure_cache = nullptr) const {
            return this->draw(bounds,(text_handle)text,strlen(text),callback,callback_state,tab_width,encoding,draw_cache,measure_cache);
        }
        inline gfx_result draw(const srect16& bounds, const text_info& ti, font_draw_callback callback, void* callback_state=nullptr) const {
            return this->draw(bounds,ti.text,ti.text_byte_count,callback,callback_state,ti.tab_width,*ti.encoding,ti.draw_cache,ti.measure_cache);
        }
    };
    
}
#endif // HTCW_GFX_FONT_HPP