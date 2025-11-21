#include "myHeap.h"
#include <stdio.h>

int main() {
    printf("=== Memory Allocator Real Merge Test ===\n\n");

    // 1. 큰 덩어리 하나를 먼저 할당받습니다. (이때 mmap 1회 호출)
    printf("[Step 1] Allocating Big Chunk (1024 bytes)\n");
    char* big_ptr = (char*)my_malloc(1024);
    
    // 2. 바로 해제합니다. 이제 Free List에는 1024짜리 큰 블록 하나가 있습니다.
    my_free(big_ptr);
    printf("Freed Big Chunk. List status:\n");
    debug_print_free_list();

    // 3. 이제 그 큰 덩어리를 쪼개서 3개를 할당합니다. (Split 발생)
    printf("\n[Step 2] Splitting Big Chunk into A, B, C\n");
    char* a = (char*)my_malloc(100); // 1024 덩어리 내부에서 쪼개짐
    char* b = (char*)my_malloc(100); // 그 바로 뒤에서 쪼개짐 (물리적 인접!)
    char* c = (char*)my_malloc(100); 
    
    printf("Allocated A, B, C inside the big chunk.\n");
    debug_print_free_list(); // 남은 자투리 공간이 보일 것임

    // 4. A와 C를 해제합니다. (가운데 B가 막고 있어서 병합 안 됨)
    printf("\n[Step 3] Freeing A and C\n");
    my_free(a);
    my_free(c);
    debug_print_free_list();

    // 5. 대망의 병합 테스트! B를 해제하면 A+B+C가 합쳐져야 함
    printf("\n[Step 4] Freeing B... Expecting Merge!\n");
    my_free(b);
    
    printf("List should show one merged block (approx 300+ bytes):\n");
    debug_print_free_list();

    return 0;
}