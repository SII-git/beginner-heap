#ifdef HEAP_C
#include "heap_c.h"

#endif

#ifdef HEAP_CPP
#include "heap_cpp.h"
#endif

#include <stdio.h>
#include <string.h>

int main() {
    #ifdef HEAP_C
    heap_init();
    #endif

    printf("--- Slot Allocation Test ---\n");
    // 슬롯 할당 테스트
    void* slot_heap[16][16] = {0,};
    for(int i = 0; i < 16; i++) {
        slot_heap[i][0] = heap_alloc(16 + i * 16); // 16, 32, 48, ..., 256 크기 할당
        if (slot_heap[i][0] == NULL) {
            fprintf(stderr, "Slot allocation failed for size %d\n", 16 + i * 16);
            return 1;
        }
        printf("heap_alloc(%d) -> %p\n", 16 + i * 16, slot_heap[i][0]);
    }

    // 슬롯 해제 테스트
    for(int i = 0; i < 16; i++) {
        heap_free(slot_heap[i][0]);
        slot_heap[i][0] = NULL; 
    }

    printf("\n--- Slot Re-allocation Test ---\n");
    // 같은 크기 슬롯 재할당 테스트
    for(int i = 0; i < 16; i++) {
        slot_heap[0][i] = heap_alloc(16); // 16 크기 할당
        if (slot_heap[0][i] == NULL) {
            fprintf(stderr, "Reallocation in slot failed\n");
            return 1;
        }
        printf("heap_alloc(16) -> %p\n", slot_heap[0][i]);
    }

    for(int i = 0; i < 16; i++) {
        heap_free(slot_heap[0][i]);
        slot_heap[0][i] = NULL; 
    }

    printf("\n--- Large Chunk Allocation/Free Test ---\n");
    // 큰 크기 할당 및 해제 테스트 (large chunk)
    void* large_chunk = heap_alloc(4096); // 4KB 할당
    if (large_chunk == NULL) {
        fprintf(stderr, "Large chunk allocation failed\n");
        return 1;
    }
    printf("heap_alloc(4096) -> %p\n", large_chunk);
    heap_free(large_chunk);
    large_chunk = NULL;

    printf("\n--- Large Chunk Splitting Test ---\n");
    // 큰 청크 분할 할당 및 해제 테스트
    void* large_chunk2 = heap_alloc(4096); // 4KB 할당
    if (large_chunk2 == NULL) {
        fprintf(stderr, "Large chunk allocation for splitting failed\n");
        return 1;
    }
    printf("heap_alloc(4096) for splitting -> %p\n", large_chunk2);

    void* sub_chunk1 = heap_alloc(1024); // 1KB 할당 (분할된 청크에서)
    if (sub_chunk1 == NULL) {
        fprintf(stderr, "Sub chunk allocation from split failed\n");
        return 1;
    }
    printf("heap_alloc(1024) from split -> %p\n", sub_chunk1);

    heap_free(large_chunk2); // 원래 청크 해제
    heap_free(sub_chunk1);   // 분할된 청크 해제
    large_chunk2 = NULL;
    sub_chunk1 = NULL;

    // Middle allocation test already has prints.
    printf("\n--- Large Chunk Middle Allocation Test ---\n");
    // 1. 다양한 크기의 청크들을 할당하여 free list를 복잡하게 만듭니다.
    void* p1 = heap_alloc(1024);
    printf("p1 = heap_alloc(1024) -> %p\n", p1);
    void* p2 = heap_alloc(2048);
    printf("p2 = heap_alloc(2048) -> %p\n", p2);
    void* p3 = heap_alloc(1024);
    printf("p3 = heap_alloc(1024) -> %p\n", p3);

    // 2. 청크들을 free하여 free list에 넣습니다.
    // p2가 리스트의 첫 번째가 아닌 위치에 있도록 free 순서를 조정합니다.
    heap_free(p1);
    printf("heap_free(p1 -> %p)\n", p1);
    heap_free(p2);
    printf("heap_free(p2 -> %p)\n", p2);
    heap_free(p3);
    printf("heap_free(p3 -> %p)\n", p3);

    // 3. p1, p3에는 작아서 할당될 수 없지만, p2에는 맞는 크기를 요청합니다.
    // 할당자는 free list를 순회하여 중간에 있는 p2를 찾아 할당해야 합니다.
    printf("Attempting to allocate from middle of free list...\n");
    void* mid_alloc = heap_alloc(1536);
    if (mid_alloc == NULL) {
        fprintf(stderr, "Middle chunk allocation failed\n");
        return 1;
    }
    printf("mid_alloc = heap_alloc(1536) -> %p (should be from p2's space)\n", mid_alloc);

    // 4. 리스트에서 중간 청크가 빠진 후에도 나머지 청크들이 잘 할당되는지 확인합니다.
    printf("Verifying other chunks are still available...\n");
    void* p1_re = heap_alloc(1000);
    if (p1_re == NULL) { fprintf(stderr, "Re-allocation of p1 failed\n"); return 1; }
    printf("p1_re = heap_alloc(1000) -> %p\n", p1_re);
    void* p3_re = heap_alloc(1000);
    if (p3_re == NULL) { fprintf(stderr, "Re-allocation of p3 failed\n"); return 1; }
    printf("p3_re = heap_alloc(1000) -> %p\n", p3_re);

    // 5. 할당된 모든 청크들을 해제합니다.
    heap_free(mid_alloc);
    heap_free(p1_re);
    heap_free(p3_re);

    #ifdef HEAP_CPP
    // Realloc test already has prints.
    printf("\n--- Realloc Test ---\n");
    // 1. realloc으로 크기 늘리기
    char *str = (char*) heap_alloc(20);
    strcpy(str, "hello");
    printf("Initial alloc: %p, content: '%s'\n", str, str);

    char *str_realloc = (char*) heap_realloc(str, 50);
    if (str_realloc == NULL) {
        fprintf(stderr, "heap_realloc (grow) failed\n");
        return 1;
    }
    printf("Grown realloc: %p, content: '%s'\n", str_realloc, str_realloc);
    // str_realloc는 이제 "hello"를 포함해야 합니다.
    if (strcmp(str_realloc, "hello") != 0) {
        fprintf(stderr, "Content corrupted after growing realloc\n");
        return 1;
    }

    // 2. realloc으로 크기 줄이기
    char *str_shrink = (char*) heap_realloc(str_realloc, 30);
    if (str_shrink == NULL) {
        fprintf(stderr, "heap_realloc (shrink) failed\n");
        return 1;
    }
    // 메모리 주소는 같거나 다를 수 있지만, 내용은 보존되어야 합니다.
    printf("Shrink realloc: %p, content: '%s'\n", str_shrink, str_shrink);
    if (strcmp(str_shrink, "hello") != 0) {
        fprintf(stderr, "Content corrupted after shrinking realloc\n");
        return 1;
    }

    // 3. realloc(ptr, 0)은 free와 같아야 함
    void* should_be_null = heap_realloc(str_shrink, 0);
    if (should_be_null != NULL) {
        fprintf(stderr, "realloc with size 0 did not return NULL\n");
        return 1;
    }
    printf("realloc with size 0 returned NULL as expected.\n");

    // 4. realloc(NULL, size)는 alloc과 같아야 함
    void* new_alloc = heap_realloc(NULL, 100);
    if (new_alloc == NULL) {
        fprintf(stderr, "realloc with NULL pointer failed\n");
        return 1;
    }
    printf("realloc with NULL pointer allocated new memory at %p.\n", new_alloc);
    heap_free(new_alloc);
    #endif

    printf("\nAll tests completed successfully!\n");
    return 0;
    }