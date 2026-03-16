// Created by RED on 11.03.2026.
#pragma once

#include "utils/memory_debugger.h"

inline void mp_init() {
    memory_debug_init();
}

inline void mp_shutdown() {
    memory_debug_shutdown();
}

