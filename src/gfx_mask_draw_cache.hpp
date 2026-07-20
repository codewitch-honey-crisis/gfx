#ifndef HTCW_GFX_MASK_DRAW_CACHE_HPP
#define HTCW_GFX_MASK_DRAW_CACHE_HPP
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
namespace gfx {
// A growable scratch buffer for anti-aliased mask/coverage drawing.
//
// It owns its storage and manages it with RAII, allocating through
// caller-supplied routines so that devices with multiple heaps can direct the
// buffer to a specific heap (e.g. PSRAM vs internal SRAM). The routines default
// to malloc/realloc/free. Pass a single instance to several draw calls to reuse
// (and keep) one buffer across them.
class mask_draw_cache final {
public:
    typedef void* (*allocator_type)(size_t);
    typedef void* (*reallocator_type)(void*, size_t);
    typedef void  (*deallocator_type)(void*);
private:
    allocator_type m_allocator;
    reallocator_type m_reallocator;
    deallocator_type m_deallocator;
    uint8_t* m_begin;
    size_t m_capacity;
    // owns a resource: non-copyable
    mask_draw_cache(const mask_draw_cache&) = delete;
    mask_draw_cache& operator=(const mask_draw_cache&) = delete;
public:
    // Frees the buffer now, but keeps the instance usable: a later ensure()
    // simply allocates again. Safe to call more than once, and safe on an
    // instance that never allocated. Does not allocate.
    void release() {
        if (nullptr != m_begin && nullptr != m_deallocator) {
            m_deallocator(m_begin);
        }
        m_begin = nullptr;
        m_capacity = 0;
    }
    mask_draw_cache(allocator_type allocator = ::malloc,
                    reallocator_type reallocator = ::realloc,
                    deallocator_type deallocator = ::free)
        : m_allocator(allocator),
          m_reallocator(reallocator),
          m_deallocator(deallocator),
          m_begin(nullptr),
          m_capacity(0) {
    }
    ~mask_draw_cache() {
        release();
    }
    mask_draw_cache(mask_draw_cache&& rhs) noexcept
        : m_allocator(rhs.m_allocator),
          m_reallocator(rhs.m_reallocator),
          m_deallocator(rhs.m_deallocator),
          m_begin(rhs.m_begin),
          m_capacity(rhs.m_capacity) {
        rhs.m_begin = nullptr;
        rhs.m_capacity = 0;
    }
    mask_draw_cache& operator=(mask_draw_cache&& rhs) noexcept {
        if (this != &rhs) {
            release();
            m_allocator = rhs.m_allocator;
            m_reallocator = rhs.m_reallocator;
            m_deallocator = rhs.m_deallocator;
            m_begin = rhs.m_begin;
            m_capacity = rhs.m_capacity;
            rhs.m_begin = nullptr;
            rhs.m_capacity = 0;
        }
        return *this;
    }
    // Ensures the buffer holds at least `size` bytes, growing it if necessary.
    // Returns the buffer, or nullptr on allocation failure. On failure the
    // previous buffer (if any) is left intact, per realloc semantics.
    uint8_t* ensure(size_t size) {
        if (size <= m_capacity) {
            return m_begin;
        }
        void* p;
        if (nullptr == m_begin) {
            if (nullptr == m_allocator) return nullptr;
            p = m_allocator(size);
        } else {
            if (nullptr == m_reallocator) return nullptr;
            p = m_reallocator(m_begin, size);
        }
        if (nullptr == p) {
            return nullptr;
        }
        m_begin = (uint8_t*)p;
        m_capacity = size;
        return m_begin;
    }
    inline uint8_t* data() { return m_begin; }
    inline const uint8_t* data() const { return m_begin; }
    inline size_t capacity() const { return m_capacity; }
};
}
#endif
