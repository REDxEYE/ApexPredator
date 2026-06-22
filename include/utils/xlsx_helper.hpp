//
// Created by red_eye on 6/21/26.
//

#pragma once
#include "OpenXLSX.hpp"
#include <span>

struct CellColors {
    std::uint32_t bg;
    std::uint32_t fg;
};

class XlsxStyleCache {
public:
    XlsxStyleCache(
        OpenXLSX::XLDocument &doc,
        std::span<const CellColors> colors
    );

    void apply(
        const OpenXLSX::XLWorksheet &sheet,
        std::uint32_t x,
        std::uint32_t y,
        std::size_t style_index
    ) const;

private:
    static std::string to_argb(std::uint32_t rgb);

    OpenXLSX::XLStyleIndex create_style(
        std::uint32_t bg,
        std::uint32_t fg
    );

    OpenXLSX::XLDocument &m_doc;
    std::vector<OpenXLSX::XLStyleIndex> m_styles;
};
