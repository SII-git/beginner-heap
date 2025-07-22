#include <sys/mman.h>
#include <stddef.h>

#include "heap_c.h"

#define INIT_SIZE 2ull*1024*1024
#define HDR_SIZE sizeof(size_t)
#define CHUNK_MIN_SIZE 0x18

typedef struct free_list {
    size_t size;
    struct free_list *next;
    struct free_list *prev;
} free_list;

typedef struct {
    char *addr;
    size_t size;
    char *top;
    free_list *free_head;
} heap_area;

static heap_area heap;


int heap_init() {
    heap.addr = mmap(NULL, INIT_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (heap.addr == MAP_FAILED) {
        return -1;
    }
    heap.size = INIT_SIZE;
    heap.free_head = NULL;
    heap.top = heap.addr;
    return 0;
}

void *heap_alloc(size_t size) {
    size = (size+7)&~7;
    
    if(size < CHUNK_MIN_SIZE) {
        size = CHUNK_MIN_SIZE;
    } else if(size > INIT_SIZE) {
        return NULL;
    }


    char* chunk;
    if(heap.free_head != NULL){
        free_list* free_chunk = heap.free_head;
        do {
            if(free_chunk->size == size) {
                chunk = (void*)free_chunk;
                if(free_chunk->prev != NULL)
                    free_chunk->prev->next = free_chunk->next;
                if(free_chunk->next != NULL) 
                    free_chunk->next->prev = free_chunk->prev;
                return chunk+HDR_SIZE;
            }
            free_chunk = free_chunk->next;
        } while(free_chunk != NULL);
    }

    chunk = heap.top;
    *(size_t*)chunk = size;
    heap.top += size + HDR_SIZE;
    return chunk+HDR_SIZE;
}

void heap_free(void *ptr) {
    free_list* chunk = ptr - HDR_SIZE;
    
    chunk->next = heap.free_head;
    chunk->prev = NULL;
    if(heap.free_head != NULL)
        heap.free_head->prev = chunk;
    heap.free_head = chunk;
}
