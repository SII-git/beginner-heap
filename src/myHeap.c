#include "myHeap.h"
#include <sys/mman.h>
#include <stdio.h>

/*    *** 구현 목표 ***
 *
 *    청크 병합
 *    bin
 *    page alignment
 *    (실제 mmap는 페이지 단위 (약 4KB)로 할당하는데 고려되어있지 않음)
 *    아레나 헤더
 *    Top chunnk(sbrk)
 * 
 * 
*/


// 청크를 분할할 때의 최소 크기
#define MIN_SIZE (sizeof(block_t) + 8) 

typedef struct block{
    size_t size; // 헤더 포함 크기   
    size_t prev_size; // 이전 블록이 free면 이전 블록의 size 저장, 할당 중이면 0
    int free;    // 0 : 사용중, 1 : Free 
    struct block* next; // fd
    struct block* prev; // bk
} block_t;


// 자유목록 이중연결리스트
static block_t* g_free_list_head = NULL;


// 자유목록에 여유가 있는지 확인
block_t* find_free_block(size_t size){
    block_t* now_block = g_free_list_head;
    while(now_block){
        if(now_block->free == 1 && now_block->size >= size) return now_block;
        now_block = now_block->next;
    }
    return NULL;
}

// 자유목록에서 청크 제거
void unlink_free_list(block_t* block){
    if (block->prev) block->prev->next = block->next;
    if (block->next) block->next->prev = block->prev;
    if (block == g_free_list_head) g_free_list_head = block->next;

    block->next = NULL;
    block->prev = NULL;
}


void split_block(block_t* block, size_t total_size){

    // 쪼개진 후 자유목록에 남아있는 블럭
    block_t* remain_block = (block_t*)((char*)block + total_size);

    remain_block->free = 1;
    remain_block->prev_size = 0;
    remain_block->size = block->size - total_size;

    // 자유목록에서 block 대신 remain_block 삽입
    remain_block->next = block->next;
    remain_block->prev = block->prev;
    if (block->prev) block->prev->next = remain_block;
    if (block->next) block->next->prev = remain_block;
    if (block == g_free_list_head) g_free_list_head = remain_block;

    // 물리적으로 next에 위치한 블럭의 prev_size를 remain__block의 size로 업데이트
    block_t* next_remain_block = (block_t*)((char*)remain_block + remain_block->size);
    next_remain_block->prev_size = remain_block->size;

    // 할당될 block 반환하기 전 전처리
    block->free = 0;
    block->prev = NULL;
    block->next = NULL;
    block->size = total_size;

}

block_t* call_mmap(size_t total_size){

    // 센티넬 블럭의 크기를 더한 총 크기
    size_t request_size = total_size + sizeof(block_t);

    /*  *** mmap() 설명 ***
    * MMAP - Memory Map 
    *
    * 1번인자 : 메모리 주소 - 특정 주소 지정 가능 / NULL 입력시 OS가 빈 공간 알아서 할당
    * 2번인자 : 메모리 크기
    * 3번인자 : 메모리 권한 : PROT_READ | PROT_WRITE | PROT_EXEC (읽기, 쓰기, 실행)
    * 4번인자 : 메모리 종류 : MAP_PRIVATE(이 프로세스에서만 사용), MAP_SHARED (여러 프로그램에서 공유 가능)
    *                        MAP_ANONYMOUS (파일이 아니라 메모리 할당할 때)
    * 5번인자 : 파일 디스크립터 (메모리 할당 시 -1)
    * 6번인자 : 파일 오프셋 (메모리 할당 시 0)
    */
    void* block_ptr = mmap(NULL, request_size, PROT_READ | PROT_WRITE, 
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (block_ptr == MAP_FAILED) return NULL; 

    block_t* new_block = (block_t*)block_ptr;
    new_block->size = total_size;
    new_block->free = 0;
    new_block->next = NULL;
    new_block->prev = NULL;
    new_block->prev_size = 0;

    // 할당된 메모리의 끝을 나타내는 센티넬 블럭 설정
    block_t* last_block = (block_t*)((char*)new_block + new_block->size);
    last_block->size = 0;
    last_block->free = 0;
    last_block->prev_size = total_size;

    return new_block;
}

void merge(block_t* block){

    block_t* next_block = (block_t*)((char*)block + block->size);
    block_t* prev_block = NULL;
    // prev_block이 free일때만 prev_block 설정 (block이 맨 첫번째 블럭일때 에러 방지)
    if(block->prev_size > 0){
        prev_block = (block_t*)((char*)block - block->prev_size);
    }

    block->free = 1;
    // prev블럭과 결합 여부
    int mergePrev = 0;
    block_t* current_block = block;

    if(next_block->free == 1 && next_block->size > 0){
        current_block->size += next_block->size;
        unlink_free_list(next_block);
    }

    if(prev_block && prev_block->free == 1){
        prev_block->size += current_block->size;
        current_block = prev_block;
        mergePrev = 1;
    }

    if (mergePrev == 0){
        current_block->next = g_free_list_head;
        current_block->prev = NULL;
        if(g_free_list_head) g_free_list_head->prev = current_block;
        g_free_list_head = current_block;
    }
   
    block_t* new_next = (block_t*)((char*)current_block + current_block->size);
    new_next->prev_size = current_block->size; 
}

void *my_malloc(size_t size){
    if(size == 0){
        return 0;
    }

    size_t total_size = size + sizeof(block_t);
    block_t* found_block = find_free_block(total_size);

    if (found_block){
        if(found_block->size - total_size >= MIN_SIZE){ // 분할 가능할 경우
            split_block(found_block, total_size);
        } else{ // 분할 불가능한 경우

            // 자유목록에서 found_block 제거
            unlink_free_list(found_block);

            found_block->free = 0;
            found_block->prev = NULL;
            found_block->next = NULL;

            // 물리적으로 next에 위치한 블럭의 prev_size 0으로 업데이트 (할당중)
            block_t* next_block = (block_t*)((char*)found_block + found_block->size);
            next_block->prev_size = 0;
        }
        return (void*)(found_block + 1);

    } else {  //자유목록에서 찾지 못했을 경우 mmap() 호출
        block_t* new_block = call_mmap(total_size);
        return (void*)(new_block + 1);
    }   
}

void my_free(void* ptr){
    
    if(ptr == NULL) return;

    //블럭의 헤더 위치로 이동
    block_t* block = (block_t*)ptr -1;
    
    merge(block);
}


// 자유목록 출력 (test용)
void debug_print_free_list() {
    block_t* curr = g_free_list_head;
    printf("[Free List]: ");
    if (!curr) {
        printf("Empty\n");
        return;
    }
    while (curr) {
        printf("[%p : size %zu] -> ", curr, curr->size);
        curr = curr->next;
    }
    printf("NULL\n");
}