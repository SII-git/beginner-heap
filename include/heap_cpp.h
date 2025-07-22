#pragma once

#include <cstddef>

__attribute__((visibility("default")))
extern void* heap_alloc(size_t size);

__attribute__((visibility("default")))
extern void heap_free(void* ptr);

__attribute__((visibility("default")))
extern void* heap_realloc(void* ptr, size_t size);