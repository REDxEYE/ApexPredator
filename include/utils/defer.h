// Created by RED on 22.09.2025.

#ifndef APEXPREDATOR_DEFER_H
#define APEXPREDATOR_DEFER_H

#define DEFER(NAME) \
for (int _defer_once_##NAME = 1, _defer_ok_##NAME = 1; _defer_once_##NAME; _defer_once_##NAME = 0) \
for (; _defer_ok_##NAME; _defer_ok_##NAME = 0)

#define DEFER_FAIL(NAME) do { _defer_ok_##NAME = 0; break; } while (0)

/* Runs CLEANUP after the body; if body called DEFER_FAIL(NAME), returns ERRVAL */
#define DEFER_CLEANUP(NAME, CLEANUP, ERRVAL) \
do { CLEANUP; if (!_defer_ok_##NAME) return (ERRVAL); } while (0)

/* Optional: same but no auto-return */
#define DEFER_CLEANUP_NORETURN(NAME, CLEANUP) \
do { CLEANUP; } while (0)

#define DEFER_SUCCEEDED(NAME) (_defer_ok_##NAME != 0)

#endif //APEXPREDATOR_DEFER_H