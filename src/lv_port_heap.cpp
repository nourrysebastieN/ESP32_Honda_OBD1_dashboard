/**
 * @file lv_port_heap.cpp
 * @brief Memory helpers for LVGL (PSRAM-first on ESP32)
 */

#include "lv_port_heap.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>

namespace {
constexpr uint32_t LV_MEM_CAPS = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
constexpr uint32_t LV_MEM_CAPS_FALLBACK = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;

void* heap_caps_malloc_with_fallback(size_t size) {
    void* buf = heap_caps_malloc(size, LV_MEM_CAPS);
    if (buf == nullptr) {
        buf = heap_caps_malloc(size, LV_MEM_CAPS_FALLBACK);
    }
    return buf;
}

void* heap_caps_realloc_with_fallback(void* ptr, size_t new_size) {
    void* buf = heap_caps_realloc(ptr, new_size, LV_MEM_CAPS);
    if (buf == nullptr && new_size != 0U) {
        buf = heap_caps_realloc(ptr, new_size, LV_MEM_CAPS_FALLBACK);
    }
    return buf;
}
} // namespace

void* lv_port_malloc(size_t size) {
    return heap_caps_malloc_with_fallback(size);
}

void lv_port_free(void* ptr) {
    if (ptr != nullptr) {
        heap_caps_free(ptr);
    }
}

void* lv_port_realloc(void* ptr, size_t new_size) {
    return heap_caps_realloc_with_fallback(ptr, new_size);
}

void* lv_port_heap_alloc_pool(size_t size) {
    static void* pool_ptr = nullptr;
    if (pool_ptr == nullptr) {
        pool_ptr = heap_caps_malloc_with_fallback(size);
    }
    return pool_ptr;
}

#else

#include <cstdlib>

void* lv_port_malloc(size_t size) {
    return std::malloc(size);
}

void lv_port_free(void* ptr) {
    std::free(ptr);
}

void* lv_port_realloc(void* ptr, size_t new_size) {
    return std::realloc(ptr, new_size);
}

void* lv_port_heap_alloc_pool(size_t size) {
    static void* pool_ptr = nullptr;
    if (pool_ptr == nullptr) {
        pool_ptr = std::malloc(size);
    }
    return pool_ptr;
}

#endif
