#include "exporter/adf_export.h"
#include "apex/adf/generated/adf_types.h"
#include "apex/hashes.h"
#include "exporter/apex/adf_dialogue.hpp"
#include "utils/hash_helper.h"
#include "utils/xlsx_helper.hpp"
#include "OpenXLSX.hpp"
#include "redscore/platform/logger.h"

#include <filesystem>
#include <ranges>

using namespace ADFTypes;

std::optional<AdfAction> export_game_adf_instance(ApexAppState &app_state, ADF::ADFFile &adf,
                                                   int &instance_id, const uint64 path_hash) {
    const auto &instances = adf.instances();
    const auto type_hash = instances[instance_id].type_hash;
    if (type_hash == std::to_underlying(ADFHashes::XLSBook)) {
        if (instances.size() != 1) {
            throw std::runtime_error("ADF with XLSBook should have only one instance");
        }
        const auto xls_book = adf.read_instance<XLSBook>(instance_id);
        auto path = find_asset_name(path_hash).value_or(std::format("unknown_{:08X}", path_hash));
        path += std::format("_{:08X}", type_hash);
        std::filesystem::path dump_path = app_state.export_path() / path;
        std::filesystem::create_directories(dump_path.parent_path());
        dump_path.replace_extension("xlsx");

        OpenXLSX::XLDocument doc;
        doc.create(dump_path.string(), OpenXLSX::XLForceOverwrite);
        auto workbook = doc.workbook();
        auto root_sheet = workbook.worksheet("Sheet1");
        std::vector<CellColors> cell_colors;
        for (const auto &attribute: xls_book->Attribute) {
            u32 bg_color = 0xDEADBEEF;
            u32 fg_color = 0xDEADBEEF;
            if (attribute.BGColorIndex < xls_book->ColorData.size()) {
                bg_color = xls_book->ColorData[attribute.BGColorIndex - 1];
            }
            if (attribute.FGColorIndex < xls_book->ColorData.size()) {
                fg_color = xls_book->ColorData[attribute.FGColorIndex - 1];
            }
            cell_colors.emplace_back(bg_color, fg_color);
        }
        XlsxStyleCache styles(doc, cell_colors);
        for (const auto &[i, sheet]: xls_book->Sheet | std::views::enumerate) {
            auto safe_name = std::format("Sheet {:02}", i);
            root_sheet.cell(i + 1, 1).value() = safe_name + " = ";
            root_sheet.cell(i + 1, 2).value() = sheet.Name;
            workbook.addWorksheet(safe_name);
            auto wks = workbook.worksheet(safe_name);
            const auto cols = sheet.Cols;
            for (const auto &[cell_id, cell_index]: sheet.CellIndex | std::views::enumerate) {
                const auto row = cell_id / cols;
                const auto col = cell_id % cols;
                const auto &cell_data = xls_book->Cell[cell_index];
                auto cell = wks.cell(row + 1, col + 1);
                auto &cell_value = cell.value();
                switch (cell_data.Type) {
                    case 0:
                        break;
                    case 1:
                        if (cell_data.DataIndex >= xls_book->StringData.size()) {
                            cell_value = "<STRING INDEX OUT OF RANGE>";
                        } else {
                            cell_value = xls_book->StringData[cell_data.DataIndex];
                        }
                        break;
                    case 2:
                    case 5:
                        if (cell_data.DataIndex >= xls_book->ValueData.size()) {
                            cell_value = "<VALUE INDEX OUT OF RANGE>";
                        } else {
                            cell_value = xls_book->ValueData[cell_data.DataIndex];
                        }
                        break;
                    default:
                        cell_value = std::format("<UNKNOWN CELL TYPE {}>", cell_data.Type);
                        GLog_Error("Unknown cell type: {}", cell_data.Type);
                }
                styles.apply(wks, row + 1, col + 1, cell_data.AttributeIndex);
            }
        }
        doc.save();
        return AdfAction{};
    }
    if (type_hash == std::to_underlying(ADFHashes::StringLookup)) {
        if (instances.size() != 1) {
            throw std::runtime_error("ADF with StringLookup should have only one instance");
        }
        const auto lookup = adf.read_instance<StringLookup>(instance_id);
        export_apex_dialogue<true>(app_state, *lookup, path_hash, type_hash);
        return AdfAction{};
    }
    return export_apex_adf_instance(app_state, adf, instance_id, path_hash);
}
