#include <gfx_font.hpp>
using namespace gfx;
int font_draw_cache::hash_function(const int32_t& key) {
    return (int)(key+5);
}
void font_draw_cache::expire_memory(size_t new_data_size) {
    while(m_cache.size() && (m_memory_size+new_data_size)>m_max_memory_size) {
        int min = -1;
        size_t sz;
        uint8_t* data;
        map_t::key_type k;
        for(size_t i = 0;i<m_cache.size();++i) {
            map_t::value_type& v = *m_cache.at(i);
            if(min==-1 || v.value.accessed<min) {
                min = v.value.accessed;
                k=v.key;
                sz = v.value.dimensions.width*v.value.dimensions.height + sizeof(cache_entry_t)+sizeof(int32_t);
                data = v.value.data;
            }  
        }
        if(min==-1) {
            break;
        }
        m_memory_size-=sz;
        free(data);
        m_cache.remove(k);   
    }
    if(!m_cache.size()) {
        m_accessed=-1;
    }
}
void font_draw_cache::expire_item() {
    if(m_cache.size()) {
        int min = -1;
        size_t sz=0;
        uint8_t* data=nullptr;
        map_t::key_type k;
        for(size_t i = 0;i<m_cache.size();++i) {
            map_t::value_type& v = *m_cache.at(i);
            if(min==-1 || v.value.accessed<min) {
                min = v.value.accessed;
                k=v.key;
                sz = v.value.dimensions.width*v.value.dimensions.height + sizeof(cache_entry_t)+sizeof(int32_t);
                data = v.value.data;
                break;
            }  
        }
        m_memory_size-=sz;
        if(data!=nullptr) free(data);
        m_cache.remove(k);
        if(!m_cache.size()) {
            m_accessed=-1;
        }
    }
}
void font_draw_cache::reduce(int new_size, int new_items) {
    if(new_size>-1) {
        while(m_cache.size() && m_memory_size>(size_t)new_size) {
            int min = -1;
            size_t sz;
            map_t::key_type k;
            uint8_t* data;
            for(size_t i = 0;i<m_cache.size();++i) {
                map_t::value_type& v = *m_cache.at(i);
                data = v.value.data;
                if(min==-1 || v.value.accessed<min) {
                    min = v.value.accessed;
                    k=v.key;
                    sz = v.value.dimensions.width*v.value.dimensions.height + sizeof(cache_entry_t)+sizeof(int32_t);
                    break;
                }
            }
            if(min==-1) {
                break;
            }
            m_memory_size-=sz;
            free(data);
            m_cache.remove(k);   
        }
    }
    if(new_items>-1) {
        while(m_cache.size() >(size_t)new_items) {
            int min = -1;
            size_t sz;
            map_t::key_type k;
            uint8_t* data;
            for(size_t i = 0;i<m_cache.size();++i) {
                map_t::value_type& v = *m_cache.at(i);
                data = v.value.data;
                if(min==-1 || v.value.accessed<min) {
                    min = v.value.accessed;
                    k=v.key;
                    sz = v.value.dimensions.width*v.value.dimensions.height + sizeof(cache_entry_t)+sizeof(int32_t);
                    break;
                }
            }
            if(min==-1) {
                break;
            }
            m_memory_size-=sz;
            free(data);
            m_cache.remove(k);   
        }
    }
    if(!m_cache.size()) {
        m_accessed=-1;
    }
}
font_draw_cache::font_draw_cache(void*(allocator)(size_t), void*(reallocator)(void*,size_t), void(deallocator)(void*)) : 
                                m_allocator(allocator),
                                m_reallocator(reallocator),
                                m_deallocator(deallocator),
                                m_initialized(false),
                                m_accessed(-1),
                                m_cache(hash_function,allocator,reallocator,deallocator),
                                m_memory_size(0), 
                                m_max_memory_size(0),
                                m_max_entries(0) {}
font_draw_cache::font_draw_cache(font_draw_cache&& rhs) : m_allocator(rhs.m_allocator),
                                m_reallocator(rhs.m_reallocator),
                                m_deallocator(rhs.m_deallocator),
                                m_initialized(rhs.m_initialized),
                                m_accessed(rhs.m_accessed),
                                m_cache(rhs.m_cache),
                                m_memory_size(rhs.m_memory_size), 
                                m_max_memory_size(rhs.m_max_memory_size),
                                m_max_entries(rhs.m_max_entries) {
}
font_draw_cache::~font_draw_cache() {
    deinitialize();
}
font_draw_cache& font_draw_cache::operator=(font_draw_cache&& rhs) {
    deinitialize();
    m_allocator=rhs.m_allocator;
    m_reallocator=rhs.m_reallocator;
    m_deallocator=rhs.m_deallocator;
    m_initialized=rhs.m_initialized;
    m_accessed=rhs.m_accessed;
    m_cache=helpers::gfx_move(rhs.m_cache);
    m_memory_size=rhs.m_memory_size;
    m_max_memory_size=rhs.m_max_memory_size;
    m_max_entries = rhs.m_max_entries;
    return *this;
}
size_t font_draw_cache::max_memory_size() const {
    return m_max_memory_size;
}
void font_draw_cache::max_memory_size(size_t value) {
    if(value<1) {
        return;
    }
    if(m_memory_size>value) {
        reduce(value,-1);
    }
    m_max_memory_size = value;
}
size_t font_draw_cache::memory_size() const {
    return m_memory_size;
}
size_t font_draw_cache::max_entries() const {
    return m_max_entries;
}
void font_draw_cache::max_entries(size_t value) {
    if(m_cache.size()>value) {
        reduce(-1,value);
    }
    m_max_entries = value;
}
size_t font_draw_cache::entries() const {
    return m_cache.size();
}

gfx_result font_draw_cache::find(int32_t codepoint, size16* out_dimensions,uint8_t** out_bitmap) {
    cache_entry_t* entry = m_cache.find(codepoint);
    if(entry==nullptr) {
        return gfx_result::canceled;
    }
    entry->accessed = m_accessed+1;
    *out_bitmap=entry->data;
    *out_dimensions=entry->dimensions;
    return gfx_result::success;
}
void font_draw_cache::clear() {
    for(size_t i = 0;i<m_cache.size();++i) {
        uint8_t* p = m_cache.at(i)->value.data;
        if(p!=nullptr) {
            m_deallocator(p);
        }
    }
    m_cache.clear();
    m_accessed=-1;
    m_memory_size=0;
}
gfx::gfx_result font_draw_cache::initialize() {
    m_accessed=-1;
    m_memory_size=0;
    m_initialized=true;
    return gfx_result::success;
}
bool font_draw_cache::initialized() const {
    return m_initialized;
}
void font_draw_cache::deinitialize() {
    clear();
    m_initialized= false;
}
gfx_result font_draw_cache::add(int32_t codepoint, size16 dimensions, const uint8_t* data) {
    size_t sz = dimensions.width*dimensions.height;
    if(sz==0) {
        return gfx_result::invalid_argument;
    }
    sz+=sizeof(cache_entry_t);
    sz+=sizeof(int32_t);
    if(m_max_memory_size>0 && sz>m_max_memory_size) {
        return gfx_result::out_of_memory;
    }
    //printf("m_cache.size(): %d, m_size: %d, m_max_size: %d, new_size: %d\n",(int)m_cache.size(),(int)m_size,(int)m_max_size,(int)sz);
    if(m_max_memory_size>0) {
        while(m_cache.size() && m_memory_size+sz>m_max_memory_size) {
            //puts("expire");
            expire_memory(sz);
            //puts("expire complete");
        }
    }
    if(m_max_entries>0) {
        while((m_cache.size()+1)>m_max_entries) {
            expire_item();
        }
    }
    if(m_max_memory_size>0 && m_memory_size+sz>m_max_memory_size) {
        return gfx_result::out_of_memory;
    }
    cache_entry_t entry;
    entry.data = (uint8_t*)m_allocator(sz);
    if(entry.data==nullptr) {
        return gfx_result::out_of_memory;
    }
    memcpy(entry.data,data,sz);
    entry.dimensions = dimensions;
    ++m_accessed;
    entry.accessed = m_accessed;
    if(!m_cache.insert({codepoint,entry})) {
        m_deallocator(entry.data);
        return gfx_result::out_of_memory;
    }
    m_memory_size+=sz;
    return gfx_result::success;
}

int font_measure_cache::hash_function(const key_t& key) {
    int result = 0;
    const uint8_t* k = key.value;
    for(int i = 0;i<8;++i) {
        result|=~*(k++);
    }
    return result+5;
}
void font_measure_cache::make_key(int32_t codepoint1, int32_t codepoint2, key_t* key) {
    memcpy(key->value,&codepoint1,sizeof(int32_t));
    memcpy(key->value+sizeof(int32_t),&codepoint2,sizeof(int32_t));
}

void font_measure_cache::expire_memory(size_t new_data_size) {
    size_t sz=sizeof(cache_entry_t)+sizeof(key_t);
    while(m_cache.size() && (m_memory_size+new_data_size)>m_max_memory_size) {
        int min = -1;
        map_t::key_type k;
        for(size_t i = 0;i<m_cache.size();++i) {
            map_t::value_type& v = *m_cache.at(i);
            if(min==-1 || v.value.accessed<min) {
                min = v.value.accessed;
                k=v.key;
            } 
        }
        if(min==-1) {
            break;
        }
        m_memory_size-=sz;
        m_cache.remove(k);   
        if(!m_cache.size()) {
            m_accessed=-1;
        }
    }
}
void font_measure_cache::expire_item() {
    if(m_cache.size()) {
        int min = -1;
        size_t sz=0;
        map_t::key_type k;
        for(size_t i = 0;i<m_cache.size();++i) {
            map_t::value_type& v = *m_cache.at(i);
            if(min==-1 || v.value.accessed<min) {
                min = v.value.accessed;
                k=v.key;
                sz = sizeof(cache_entry_t)+sizeof(key_t);
                break;
            }  
        }
        m_memory_size-=sz;
        m_cache.remove(k);
        if(!m_cache.size()) {
            m_accessed=-1;
        }
    }
}
void font_measure_cache::reduce(int new_size, int new_items) {
    if(new_size>-1) {
        while(m_cache.size() && m_memory_size>(size_t)new_size) {
            int min = -1;
            size_t sz;
            map_t::key_type k;
            for(size_t i = 0;i<m_cache.size();++i) {
                map_t::value_type& v = *m_cache.at(i);
                if(min==-1 || v.value.accessed<min) {
                    min = v.value.accessed;
                    k=v.key;
                    sz = sizeof(cache_entry_t)+sizeof(key_t);;
                    break;
                }
            }
            if(min==-1) {
                break;
            }
            m_memory_size-=sz;
            m_cache.remove(k);   
        }
    }
    if(new_items>-1) {
        while(m_cache.size() >(size_t)new_items) {
            int min = -1;
            size_t sz;
            map_t::key_type k;
            for(size_t i = 0;i<m_cache.size();++i) {
                map_t::value_type& v = *m_cache.at(i);
                if(min==-1 || v.value.accessed<min) {
                    min = v.value.accessed;
                    k=v.key;
                    sz = sizeof(cache_entry_t)+sizeof(key_t);;
                    break;
                }
            }
            if(min==-1) {
                break;
            }
            m_memory_size-=sz;
            m_cache.remove(k);   
        }
    }
    if(!m_cache.size()) {
        m_accessed=-1;
    }
}
font_measure_cache::font_measure_cache(void*(allocator)(size_t), void*(reallocator)(void*,size_t), void(deallocator)(void*)) : 
                                m_allocator(allocator),
                                m_reallocator(reallocator),
                                m_deallocator(deallocator),
                                m_initialized(false),
                                m_accessed(-1),
                                m_cache(hash_function,allocator,reallocator,deallocator),
                                m_memory_size(0), 
                                m_max_memory_size(0),
                                m_max_entries(0) {}
font_measure_cache::font_measure_cache(font_measure_cache&& rhs) : m_allocator(rhs.m_allocator),
                                m_reallocator(rhs.m_reallocator),
                                m_deallocator(rhs.m_deallocator),
                                m_initialized(rhs.m_initialized),
                                m_accessed(rhs.m_accessed),
                                m_cache(rhs.m_cache),
                                m_memory_size(rhs.m_memory_size), 
                                m_max_memory_size(rhs.m_max_memory_size),
                                m_max_entries(rhs.m_max_entries) {
}
font_measure_cache::~font_measure_cache() {
    deinitialize();
}
font_measure_cache& font_measure_cache::operator=(font_measure_cache&& rhs) {
    deinitialize();
    m_allocator=rhs.m_allocator;
    m_reallocator=rhs.m_reallocator;
    m_deallocator=rhs.m_deallocator;
    m_initialized=rhs.m_initialized;
    m_accessed=rhs.m_accessed;
    m_cache=helpers::gfx_move(rhs.m_cache);
    m_memory_size=rhs.m_memory_size;
    m_max_memory_size=rhs.m_max_memory_size;
    m_max_entries = rhs.m_max_entries;
    return *this;
}
size_t font_measure_cache::max_memory_size() const {
    return m_max_memory_size;
}
void font_measure_cache::max_memory_size(size_t value) {
    if(value<1) {
        return;
    }
    if(m_max_memory_size==0 || m_memory_size>value) {
        reduce(value,-1);
    }
    m_max_memory_size = value;
}
size_t font_measure_cache::memory_size() const {
    return m_memory_size;
}

size_t font_measure_cache::max_entries() const {
    return m_max_entries;
}
void font_measure_cache::max_entries(size_t value) {
    if(value<1) {
        return;
    }
    if(m_max_entries==0 ||m_cache.size()>value) {
        reduce(-1,value);
    }
    m_max_entries = value;
}
size_t font_measure_cache::entries() const {
    return m_cache.size();
}

gfx_result font_measure_cache::find(int32_t codepoint1, int32_t codepoint2, font_glyph_info* out_glyph_info) {
    key_t k;
    make_key(codepoint1,codepoint2,&k);
    cache_entry_t* entry = m_cache.find(k);
    if(entry==nullptr) {
        return gfx_result::canceled;
    }
    entry->accessed = m_accessed + 1;
    *out_glyph_info = entry->data;
    return gfx_result::success;
}
void font_measure_cache::clear() {
    m_cache.clear();
    m_accessed=-1;
    m_memory_size=0;
}
gfx::gfx_result font_measure_cache::initialize() {
    m_accessed=-1;
    m_memory_size=0;
    m_initialized=true;
    return gfx_result::success;
}
bool font_measure_cache::initialized() const {
    return m_initialized;
}
void font_measure_cache::deinitialize() {
    clear();
    m_initialized= false;
}
gfx_result font_measure_cache::add(int32_t codepoint1, int32_t codepoint2, const font_glyph_info& glyph_info) {
    size_t sz = sizeof(cache_entry_t)+sizeof(key_t);;
    if(m_max_memory_size>0 && sz>m_max_memory_size) {
        return gfx_result::out_of_memory;
    }
    if(m_max_memory_size>0) {
        while( m_cache.size() && m_memory_size+sz>m_max_memory_size) {
            //printf("m_memory_size: %d, m_max_memory_size: %d, sz: %d\n",(int)m_memory_size,(int)m_max_memory_size,(int)sz);
            expire_memory(sz);
        }
        if(m_memory_size+sz>m_max_memory_size) {
            return gfx_result::out_of_memory;
        }
    }
    if(m_max_entries>0) {
        while((m_cache.size()+1)>m_max_entries) {
            expire_item();
        }
    }
    if(m_max_memory_size>0 && m_memory_size+sz>m_max_memory_size) {
        return gfx_result::out_of_memory;
    }
    cache_entry_t entry;
    entry.data = glyph_info;
    ++m_accessed;
    entry.accessed = m_accessed;
    key_t k;
    make_key(codepoint1,codepoint2,&k);
    if(!m_cache.insert({k,entry})) {
        return gfx_result::out_of_memory;
    }
    //printf("ADDED, size: %d\n",(int)m_cache.size());
    m_memory_size += sz;
    return gfx_result::success;
}
gfx_result font::measure(uint16_t max_width,const text_handle text, size_t text_length, size16* out_area, uint16_t tab_width, const text_encoder& encoding, font_measure_cache* cache) const {
    if(text==nullptr || out_area==nullptr) {
        return gfx_result::invalid_argument;
    }
    if(cache!=nullptr&&!cache->initialized()) {
        cache = nullptr;
    }
    const uint8_t* sz = (const uint8_t*)text;
    size_t len=text_length;
    out_area->width = 0;
    out_area->height = 0;
    if(len==0) {
        return gfx_result::success;
    }
    uint16_t em_width = 0;
    uint16_t x_ext = 0, y_ext = 0;
    uint16_t x = 0,y=0;
    int32_t cp=0, cp_next=0;
    size_t advlen = len;
    size_t advlen_next = 0;
    const uint16_t lineadv = this->line_advance();
    gfx_result res = encoding.to_utf32((text_handle)sz,&cp,&advlen);
    if(res!=gfx_result::success) {
        return res;
    }
    sz+=advlen;
    len-=advlen;
    if(len) {
        advlen_next = len;
        res = encoding.to_utf32((text_handle)sz,&cp_next,&advlen_next);
        if(res!=gfx_result::success) {
            return res;
        }
        sz+=advlen_next;
        len-=advlen_next;
        y_ext = lineadv;
    }
    font_glyph_info em_gi;
    uint16_t xo, cw;
    bool cached;
    int tail;
    while(1) {
        font_glyph_info gi;
        switch(cp) {
            case '\t':
                if(em_width==0) {
                    cached = false;
                    if(cache!=nullptr) {
                        if(gfx_result::success==cache->find('M',0,&em_gi)) {
                            cached = true;
                        }
                    }
                    if(!cached) {
                        res=on_measure('M',0,&em_gi);
                        if(res!=gfx_result::success) {
                            return res;
                        }
                        if(cache!=nullptr) {
                            cache->add('M',0,em_gi);
                        }
                    }
                               
                    em_width=em_gi.advance_width;
                }
                cw = em_width*tab_width;
                x=((x/cw)+1)*cw;
                if(x>=max_width) {
                    x=0;
                    y+=lineadv;
                    if(y+lineadv>y_ext) {
                        y_ext = y+lineadv;
                    }
                }
                if(x>x_ext) {
                    x_ext = x;
                }
                break;
            case '\r':
                x=0;
                xo=0;
                break;
            case '\n':
                y+=lineadv;
                if(y+lineadv>y_ext) {
                    y_ext = y+lineadv;
                }
                xo=0;
                x=0;
                break;
            default:
                cached = false;
                if(cache!=nullptr) {
                    if(gfx_result::success==cache->find(cp,(int)(cp_next>=0x20)?cp_next:0,&gi)) {
                        cached = true;
                    }
                }
                if(!cached) {
                    res = this->on_measure((int)cp,(int)(cp_next>=0x20)?cp_next:0,&gi);
                    if(res!=gfx_result::success) {
                        return res;
                    }
                    if(cache!=nullptr) {
                        cache->add((int)cp,(int)cp_next,gi);
                    }
                }
                xo=x+gi.dimensions.width+gi.offset.x;
                tail = (gi.dimensions.height+gi.offset.y)>lineadv?(gi.dimensions.height+gi.offset.y):lineadv;
                if(xo>max_width) {
                    x=0;
                    xo = gi.dimensions.width+gi.offset.x;
                    y+=lineadv;
                    if(y+tail>y_ext) {
                        y_ext = y+tail;
                    }
                }  else {
                    x+=gi.advance_width;
                }
                if(y+tail>y_ext) {
                    y_ext = y+tail;
                }
                if(xo>x_ext) {
                    x_ext = xo;
                }
                
                break;
        }
        cp = cp_next;
        if(!cp) {
            break;
        }
        // advance
        if(len) {
            advlen_next = len;
            res = encoding.to_utf32((text_handle)sz,&cp_next,&advlen_next);
            if(res!=gfx_result::success) {
                return res;
            }
            sz+=advlen_next;
            len-=advlen_next;
        } else {
            cp_next = 0;
        }
    }
    out_area->width = x_ext;
    if(out_area->width>max_width) {
        out_area->width=max_width;
    }
    out_area->height = y_ext;
    return gfx_result::success;
}

gfx_result font::draw(const gfx::srect16& bounds, const text_handle text, size_t text_length, font_draw_callback callback, void* callback_state, uint16_t tab_width, const text_encoder& encoding, font_draw_cache* draw_cache, font_measure_cache* measure_cache) const {
    using bmp_t = gfx::bitmap<alpha_pixel<8>>;
    using const_bmp_t = gfx::const_bitmap<alpha_pixel<8>>;
    if(text==nullptr || callback==nullptr) {
        return gfx_result::invalid_argument;
    }
    if(draw_cache!=nullptr&& !draw_cache->initialized()) {
        // don't bother using it if it's disabled.
        draw_cache = nullptr;
    }
    if(measure_cache!=nullptr&& !measure_cache->initialized()) {
        // don't bother using it if it's disabled.
        measure_cache = nullptr;
    }

    uint8_t* buffer = nullptr;
    size_t buffer_size = 0;
    const uint8_t* sz = (const uint8_t*)text;
    size_t len = text_length;
    if(len==0) {
        return gfx_result::success;
    }
    uint16_t em_width = 0;
    uint16_t x_ext = 0, y_ext = 0;
    uint16_t x = 0,y=0;
    int32_t cp=0, cp_next=0;
    size_t advlen = len;
    size_t advlen_next = 0;
    const uint16_t lineh = this->line_advance();
    //printf("line adv: %d\n",(int)lineh);
    gfx_result res = encoding.to_utf32((text_handle)sz,&cp,&advlen);
    if(res!=gfx_result::success) {
        return res;
    }
    sz +=advlen;
    len-=advlen;
    if(len) {
        advlen_next = len;
        res = encoding.to_utf32((text_handle)sz,&cp_next,&advlen_next);
        if(res!=gfx_result::success) {
            return res;
        }
        sz+=advlen_next;
        len-=advlen_next;
    }
    font_glyph_info em_gi;
    uint16_t xo, cw;
    bool cached;
    while(1) {
        font_glyph_info gi;
        switch(cp) {
            case '\t':
                if(em_width==0) {
                    cached = false;
                    if(measure_cache!=nullptr) {
                        if(gfx_result::success==measure_cache->find('M',0,&em_gi)) {
                            cached = true;
                        }
                    }
                    if(!cached) {
                        res=on_measure('M',0,&em_gi);
                        if(res!=gfx_result::success) {
                            if(buffer) {
                                free(buffer);
                            }
                            return res;
                        }
                        if(measure_cache!=nullptr) {
                            measure_cache->add('M',0,em_gi);
                        }
                    }
                    em_width = em_gi.advance_width;
                }
                
                cw = em_width*tab_width;
                x=((x/cw)+1)*cw;
                if(x>=bounds.width()) {
                    x=0;
                    y+=lineh;
                }
                if(x>x_ext) {
                    x_ext = x;
                }
                break;
            case '\r':
                x=0;
                break;
            case '\n':
                y+=lineh;
                x=0;
                break;
            default:
                cached = false;
                if(measure_cache!=nullptr) {
                    if(gfx_result::success==measure_cache->find(cp,(cp_next>=0x20)?cp_next:0,&gi)) {
                        cached = true;
                    }
                }
                if(!cached) {
                    res = this->on_measure((int)cp,(int)(cp_next>=0x20)?cp_next:0,&gi);
                    if(res!=gfx_result::success) {
                        if(buffer) {
                            free(buffer);
                        }
                        return res;
                    }
                    if(measure_cache!=nullptr) {
                        measure_cache->add(cp,(int)(cp_next>=0x20)?cp_next:0,gi);
                    }
                }
                
                if(cp>=0x20) { 
                    cached = false;
                    if(draw_cache!=nullptr) {
                        size16 d;
                        uint8_t * b;
                        if(gfx_result::success==draw_cache->find(cp,&d,&b)) {
                            const_bmp_t cbmp(d,b);
                            //printf("gi.offset.y: %d\n",gi.offset.y);
                            spoint16 loc = spoint16(x,y).offset(bounds.point1()).offset(gi.offset);
                            callback(loc,cbmp,callback_state);
                            cached = true;
                            //printf("(%d,%d)\n",x,y);
                            //printf("cache hit for %c (%d)\n",(int)cp,(int)cp);
                        }
                    }
                    if(!cached) {
                        // do draw
                        size_t bsize = gi.dimensions.width*gi.dimensions.height;
                        if(buffer_size==0) {
                            buffer = (uint8_t*)malloc(bsize);
                            if(buffer==nullptr) {
                                return gfx_result::out_of_memory;
                            }
                            memset(buffer,0,bsize);
                            buffer_size = bsize;
                        } else {
                            if(buffer_size<bsize) {
                                buffer = (uint8_t*)realloc(buffer,bsize);
                                if(buffer==nullptr) {
                                    return gfx_result::out_of_memory;
                                }
                                buffer_size = bsize;
                            }
                        }
                        bmp_t bmp(gi.dimensions,buffer);
                        res = on_draw(bmp,cp,gi.glyph_index1);
                        if(res!=gfx_result::success) {
                            if(buffer) {
                                free(buffer);
                            }
                            return res;
                        }
                        if(draw_cache!=nullptr) {
                            draw_cache->add(cp,gi.dimensions,bmp.begin());
                            // don't care about errors here
                        }
                        spoint16 loc = spoint16(x,y).offset(bounds.point1()).offset(gi.offset);
                        //printf("loc: (%d,%d)\n",(int)loc.x,(int)loc.y);
                        const_bmp_t cbmp(gi.dimensions,buffer);
                        callback(loc,cbmp,callback_state);
                    }
                }
                if(y+lineh>y_ext) {
                    y_ext = y+lineh;
                }
                xo=x+gi.dimensions.width+gi.offset.x;
                if(xo>bounds.width()) {
                    x=0;
                    xo = gi.dimensions.width+gi.offset.x;
                    y+=lineh;
                } else {
                    x+=gi.advance_width;
                }
                if(xo>x_ext) {
                    x_ext = xo;
                }
                break;
        }
        cp = cp_next;
        if(!cp || y>bounds.y2) {
            break;
        }
        // advance
        if(len) {
            advlen_next = len;
            res = encoding.to_utf32((text_handle)sz,&cp_next,&advlen_next);
            if(res!=gfx_result::success) {
                if(buffer) {
                    free(buffer);
                }
                return res;
            }
            sz+=advlen_next;
            len-=advlen_next;
        } else {
            cp_next = 0;
        }
    }
    if(buffer) {
        free(buffer);
    }
    return gfx_result::success;
}

const text_encoding::utf8_encoder text_encoding::s_utf8;
const text_encoding::latin1_encoder text_encoding::s_latin1;

const text_encoder& text_encoding::utf8=text_encoding::s_utf8;
const text_encoder& text_encoding::latin1=text_encoding::s_latin1;