//
// Created by red_eye on 6/21/26.
//

#include "utils/xlsx_helper.hpp"

XlsxStyleCache::XlsxStyleCache(OpenXLSX::XLDocument &doc, const std::span<const CellColors> colors): m_doc(doc) {
    m_styles.reserve(colors.size());

    for (const CellColors& color : colors) {
        m_styles.push_back(create_style(color.bg, color.fg));
    }
}

void XlsxStyleCache::apply(const OpenXLSX::XLWorksheet &sheet, const std::uint32_t x, const std::uint32_t y,
    const std::size_t style_index) const {
    // OpenXLSX uses row, column. Your X/Y likely means column/row.
    auto cell = sheet.cell(OpenXLSX::XLCellReference(y, x));
    cell.setCellFormat(m_styles.at(style_index));
}

std::string XlsxStyleCache::to_argb(const std::uint32_t rgb) {
    char buffer[9];
    std::snprintf(buffer, sizeof(buffer), "ff%06X", rgb & 0xFFFFFFu);
    return buffer;
}

OpenXLSX::XLStyleIndex XlsxStyleCache::create_style(const std::uint32_t bg, const std::uint32_t fg) {
    auto& styles = m_doc.styles();
    auto& cell_formats = styles.cellFormats();
    auto& fills = styles.fills();
    auto& fonts = styles.fonts();

    // Use default/base style as template.
    OpenXLSX::XLStyleIndex base_format = 0;

    OpenXLSX::XLStyleIndex new_format =
            cell_formats.create(cell_formats[base_format]);

    OpenXLSX::XLStyleIndex new_fill =
            fills.create(fills[cell_formats[base_format].fillIndex()]);

    OpenXLSX::XLStyleIndex new_font =
            fonts.create(fonts[cell_formats[base_format].fontIndex()]);

    fills[new_fill].setPatternType(OpenXLSX::XLPatternSolid);
    fills[new_fill].setColor(OpenXLSX::XLColor(to_argb(bg)));

    fonts[new_font].setFontColor(OpenXLSX::XLColor(to_argb(fg)));

    cell_formats[new_format].setFillIndex(new_fill);
    cell_formats[new_format].setFontIndex(new_font);

    return new_format;
}
