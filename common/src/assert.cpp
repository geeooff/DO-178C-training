#include "avio/assert.hpp"

#include <cstdio>

namespace avio {
namespace {

void default_handler(const char* condition, const char* file, i32 line) noexcept {
    std::printf("[ANOMALIE] %s  (%s:%d)\n", condition, file, static_cast<int>(line));
}

FaultHandler g_handler = &default_handler;
u32 g_fault_count = 0U;

}  // namespace

FaultHandler set_fault_handler(FaultHandler handler) noexcept {
    FaultHandler previous = g_handler;
    g_handler = (handler != nullptr) ? handler : &default_handler;
    return previous;
}

u32 fault_count() noexcept {
    return g_fault_count;
}

void reset_fault_count() noexcept {
    g_fault_count = 0U;
}

namespace detail {

void on_assert_failed(const char* condition, const char* file, i32 line) noexcept {
    g_fault_count += 1U;
    if (g_handler != nullptr) {
        g_handler(condition, file, line);
    }
}

}  // namespace detail
}  // namespace avio
