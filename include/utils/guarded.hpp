//
// Created by red_eye on 9/24/26.
//

#pragma once

#include "cstdint"

inline void message(char *out, const size_t size, const std::string_view text) noexcept {
    if (!out || !size) return;
    auto count = std::min(size - 1, text.size());
    std::memcpy(out, text.data(), count);
    out[count] = 0;
}

template<class F>
int32_t guarded(char *error, const size_t size, F &&operation) noexcept {
    message(error, size, {});
    try {
        operation();
        return 0;
    } catch (const std::exception &e) {
        message(error, size, e.what());
        return -1;
    } catch (...) {
        message(error, size, "Unknown Generation Zero module error");
        return -1;
    }
}
