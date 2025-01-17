#ifndef HTCW_GFX_TT_FONT_HPP
#define HTCW_GFX_TT_FONT_HPP
#include <gfx_font.hpp>
namespace gfx {
    class tt_font : public font {
        void* m_info;
        gfx::stream* m_stream;
        uint16_t m_line_height;
        uint16_t m_line_advance;
        uint16_t m_base_line;
        float m_scale;
        float m_size;
        font_size_units m_units;
        bool m_kerning;
        tt_font(const tt_font& rhs)=delete;
        tt_font& operator=(const tt_font& rhs)=delete;
        void set_size();
    protected:
        virtual gfx::gfx_result on_measure(int32_t codepoint1,int32_t codepoint2, font_glyph_info* out_glyph_info) const override;
        virtual gfx::gfx_result on_draw(gfx::bitmap<gfx::alpha_pixel<8>>& destination,int32_t codepoint, int32_t glyph_index = -1) const override;
    public:
        tt_font(gfx::stream& stream,float size, font_size_units units = font_size_units::em, bool initialize = false);
        tt_font();
        virtual ~tt_font();
        tt_font(tt_font&& rhs);
        tt_font& operator=(tt_font&& rhs);
        void size(float size, font_size_units units = font_size_units::em);
        virtual gfx::gfx_result initialize() override;
        virtual bool initialized() const override;
        virtual void deinitialize() override;
        virtual uint16_t line_height() const override;
        virtual uint16_t line_advance() const override;
        virtual uint16_t base_line() const override;
    };
}
#endif // HTCW_GFX_TT_FONT_HPP