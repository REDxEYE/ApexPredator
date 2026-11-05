#ifndef APEXPREDATOR_CPP_IDENTIFIER_H
#define APEXPREDATOR_CPP_IDENTIFIER_H

#include <algorithm>
#include <iterator>
#include <string>
#include <string_view>

namespace Utils {
    inline std::string cpp_identifier(const std::string_view name) {
        // Keep schema names intact for metadata and JSON; escape only C++ symbols.
        constexpr std::string_view keywords[] = {
            "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break",
            "case", "catch", "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept",
            "const", "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await", "co_return",
            "co_yield", "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum",
            "explicit", "export", "extern", "false", "float", "for", "friend", "goto", "if", "inline", "int",
            "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator",
            "or", "or_eq", "private", "protected", "public", "register", "reinterpret_cast", "requires",
            "return", "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct",
            "switch", "template", "this", "thread_local", "throw", "true", "try", "typedef", "typeid",
            "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while",
            "xor", "xor_eq", "final", "override", "import", "module"
        };
        if (std::find(std::begin(keywords), std::end(keywords), name) != std::end(keywords))
            return std::string(name) + '_';
        return std::string(name);
    }
}

#endif // APEXPREDATOR_CPP_IDENTIFIER_H
