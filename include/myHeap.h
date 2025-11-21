#ifndef MY_MALLOC_H
#define MY_MALLOC_H

#include <stddef.h>


void* my_malloc(size_t size);
void  my_free(void* ptr);

void debug_print_free_list(); 

#endif