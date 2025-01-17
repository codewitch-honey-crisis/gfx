#ifndef HTCW_GFX_WIN_FONT_HPP
#define HTCW_GFX_WIN_FONT_HPP
#include <gfx_font.hpp>
namespace gfx {
    class win_font : public font {
        gfx::stream* m_stream;
        size_t m_font_index;
        uint16_t m_line_height;
        uint16_t m_width;
        long long m_font_offset;
        uint16_t m_char_table_offset, m_char_table_len;
        char m_first_char,m_last_char;
        win_font(const win_font& rhs)=delete;
        win_font& operator=(const win_font& rhs)=delete;
        gfx::gfx_result seek_char(char ch) const;
    protected:
        virtual gfx::gfx_result on_measure(int32_t codepoint1,int32_t codepoint2, font_glyph_info* out_glyph_info) const override;
        virtual gfx::gfx_result on_draw(gfx::bitmap<gfx::alpha_pixel<8>>& destination,int32_t codepoint, int32_t glyph_index = -1) const override;
    public:
        win_font(gfx::stream& stream, size_t font_index = 0, bool initialize = false);
        win_font();
        virtual ~win_font();
        win_font(win_font&& rhs);
        win_font& operator=(win_font&& rhs);
        virtual gfx::gfx_result initialize() override;
        virtual bool initialized() const override;
        virtual void deinitialize() override;
        virtual uint16_t line_height() const override;
        virtual uint16_t line_advance() const override;
        virtual uint16_t base_line() const override;
    };
}
#endif // HTCW_GFX_WIN_FONT_HPP