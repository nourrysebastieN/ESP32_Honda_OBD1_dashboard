/**
 * @file lv_port_heap.h
 * @brief LVGL custom heap helpers
 */

#ifndef LV_PORT_HEAP_H
#define LV_PORT_HEAP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* lv_port_malloc(size_t size);
void  lv_port_free(void* ptr);
void* lv_port_realloc(void* ptr, size_t new_size);
void* lv_port_heap_alloc_pool(size_t size);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* LV_PORT_HEAP_H */
