#ifndef HTCW_GFX_DITHER_CACHE_HPP
#define HTCW_GFX_DITHER_CACHE_HPP
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace gfx {
// A growable scratch buffer that holds the state Floyd-Steinberg error
// diffusion needs for a single dither operation.
//
// Like mask_draw_cache, it owns its storage via RAII and allocates through
// caller-supplied routines so a device with multiple heaps can steer the
// buffer to a specific one (e.g. PSRAM vs internal SRAM). Routines default to
// malloc/realloc/free. Pass one instance to several dither calls to reuse the
// allocation across them (the contents are re-initialized each op, so reuse is
// purely an allocation optimization, not a way to carry error between ops).
class dither_cache final {
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
    dither_cache(const dither_cache&) = delete;
    dither_cache& operator=(const dither_cache&) = delete;
public:
    // Frees the buffer now but keeps the instance usable; a later ensure()
    // allocates again. Safe to call repeatedly and on an instance that never
    // allocated. Does not allocate.
    void release() {
        if (nullptr != m_begin && nullptr != m_deallocator) {
            m_deallocator(m_begin);
        }
        m_begin = nullptr;
        m_capacity = 0;
    }
    dither_cache(allocator_type allocator = ::malloc,
                 reallocator_type reallocator = ::realloc,
                 deallocator_type deallocator = ::free)
        : m_allocator(allocator),
          m_reallocator(reallocator),
          m_deallocator(deallocator),
          m_begin(nullptr),
          m_capacity(0) {
    }
    dither_cache(uint8_t* buffer, size_t capacity)
        : m_allocator(nullptr),
          m_reallocator(nullptr),
          m_deallocator(nullptr),
          m_begin(buffer),
          m_capacity(capacity) {
    }
    ~dither_cache() {
        release();
    }
    dither_cache(dither_cache&& rhs) noexcept
        : m_allocator(rhs.m_allocator),
          m_reallocator(rhs.m_reallocator),
          m_deallocator(rhs.m_deallocator),
          m_begin(rhs.m_begin),
          m_capacity(rhs.m_capacity) {
        rhs.m_begin = nullptr;
        rhs.m_capacity = 0;
    }
    dither_cache& operator=(dither_cache&& rhs) noexcept {
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
    // Ensures the buffer holds at least `size` bytes, growing if necessary.
    // Returns the buffer, or nullptr on allocation failure. On failure the
    // previous buffer (if any) is left intact, per realloc semantics.
    uint8_t* ensure(size_t size) {
        if (m_allocator == nullptr) {
            // wrapped external buffer: cannot grow
            if (size <= m_capacity) return m_begin;
            return nullptr;
        }
        if (size <= m_capacity) {
            return m_begin;
        }
        void* p;
        if (nullptr == m_begin) {
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

    // --- Floyd-Steinberg scratch ------------------------------------------
    // Error is carried as int32 per channel. int16 is enough for <=~14-bit
    // channels, but accumulated neighbor contributions can transiently exceed
    // that on deep channels, so int32 is the safe element and the cost (a few
    // KB of scratch) is negligible next to the destination framebuffer.
    typedef int32_t error_element;

    // Maximum bytes needed for the FS error scratch: two rows (current being
    // consumed, next being accumulated), each `width` pixels * `channels`
    // channels. Static so callers can pre-size an external buffer to wrap via
    // the (uint8_t*, size_t) constructor.
    //
    static constexpr size_t sizeof_buffer(int width, size_t channels) {
        return (size_t)width * channels * 2 * sizeof(error_element);
    }

    // Carves two zeroed error rows of `width` * `channels` out of the buffer
    // (current row being consumed + next row being accumulated). Returns false
    // on allocation failure. `curr` and `next` alias the owned buffer; do not
    // free them.
    bool error_rows(int width, size_t channels, error_element** curr, error_element** next) {
        const size_t row_elems = (size_t)width * channels;
        const size_t bytes = sizeof_buffer(width, channels);
        uint8_t* p = ensure(bytes);
        if (nullptr == p) {
            return false;
        }
        error_element* rows = (error_element*)p;
        *curr = rows;
        *next = rows + row_elems;
        memset(p, 0, bytes);
        return true;
    }
};
}  // namespace gfx
#endif