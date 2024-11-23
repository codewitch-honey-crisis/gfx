#ifndef HTCW_GFX_VLW_FONT_HPP
#define HTCW_GFX_VLW_FONT_HPP
#include <gfx_font.hpp>
namespace gfx {
    class vlw_font : public font {
        gfx::stream* m_stream;
        size_t m_glyph_count;
        uint16_t m_line_advance;
        uint16_t m_space_width;
        int16_t m_ascent, m_descent;
        int16_t m_max_ascent, m_max_descent;
        gfx::size16 m_bmp_size_max;
        gfx::gfx_result seek_codepoint(int32_t codepoint, int32_t* out_glyph_index = nullptr) const;
        gfx::gfx_result read_uint32(uint32_t* out) const;
    protected:
        virtual gfx::gfx_result on_measure(int32_t codepoint1,int32_t codepoint2, font_glyph_info* out_glyph_info) const override;
        virtual gfx::gfx_result on_draw(gfx::bitmap<gfx::alpha_pixel<8>>& destination,int32_t codepoint, int32_t glyph_index = -1) const override;
    public:
        vlw_font(gfx::stream& stream, bool initialize = false);
        vlw_font();
        virtual ~vlw_font();
        vlw_font(vlw_font&& rhs);
        vlw_font& operator=(vlw_font&& rhs);
        virtual gfx::gfx_result initialize() override;
        virtual bool initialized() const override;
        virtual void deinitialize() override;
        virtual uint16_t line_height() const override;
        virtual uint16_t line_advance() const override;
        virtual uint16_t base_line() const override;
    };
}
#endif // HTCW_GFX_VLW_FONT_HPP