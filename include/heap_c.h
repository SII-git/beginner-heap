#pragma once

#include <stddef.h>

__attribute__((visibility("default")))
extern int heap_init();

__attribute__((visibility("default")))
extern void* heap_alloc(size_t size);

__attribute__((visibility("default")))
extern void heap_free(void* ptr);
