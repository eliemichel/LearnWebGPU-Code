#pragma once

#define MOVE_ONLY(T) \
T(const T&) = delete; \
T& operator=(const T&) = delete; \
T(T&&) = default; \
T& operator=(T&&) = default;

#define UNUSED(x) ((void)x)
