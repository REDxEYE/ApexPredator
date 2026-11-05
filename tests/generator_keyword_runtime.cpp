#include "apex/adf/generated/adf_types.h"
#include "havok/generated/havok_types.h"

#include <sstream>
#include <stdexcept>

static void check(bool condition) {
    if (!condition) throw std::runtime_error("Generated keyword identifier regression failed");
}

int main() {
    ADFTypes::static_ adf{};
    adf.default_ = 11;
    adf.default__1 = 12;
    adf.static__ = 13;
    adf.kind = ADFTypes::class_::default_;
    adf.override_ = ADFTypes::class_::override_;
    adf.left.value = 14;
    adf.right.value = 15;
    const auto adf_json = adf.to_json();
    check(adf_json.at("default") == 11 && adf_json.at("default_") == 12 &&
          adf_json.at("static") == 13 && adf_json.at("override") == 2 &&
          adf_json.at("left").at("value") == 14 && adf_json.at("right").at("value") == 15);
    check(ADFTypes::static__TI.name == "static" && ADFTypes::default__TI.name == "default");
    std::ostringstream adf_print;
    adf.print(adf_print);
    check(adf_print.str().find("default: 11") != std::string::npos);

    HavokTypes::class_ havok{};
    havok.default__ = 21;
    havok.default_ = 22;
    havok.static_ = 23;
    havok.kind.value = HavokTypes::enum_{};
    const auto havok_json = havok.to_json();
    check(havok_json.at("default") == 21 && havok_json.at("default_") == 22 &&
          havok_json.at("static") == 23 && havok_json.at("kind") == 0);
    std::ostringstream havok_print;
    havok.print(havok_print);
    check(havok_print.str().find("\"default\":21") != std::string::npos);
    init_havok_type_info();
    check(havok_type_info.at(0xBA1D0001)->name == "class");
}
