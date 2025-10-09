#include "myheap.h"
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define ALIGN 16
#define ALIGN_UP(x) (((x) + (ALIGN - 1)) & ~(ALIGN - 1))
#define HEAP_SIZE (1024 * 1024)  // 1MB arena

/* ===== Block layout =====
 * [block_t header][payload...]
 * - next/prev : 힙 순서(인접 블록) 연결
 * - fnext/fprev: 사이즈 bin 내 자유목록 연결( free 일 때만 유효 )
 */
typedef struct block {
    size_t size;            // payload 크기 (aligned)
    int free;               // 1=free, 0=used
    struct block *next;     // 힙 순서 연결(오른쪽 이웃)
    struct block *prev;     // 힙 순서 연결(왼쪽 이웃)
    struct block *fnext;    // bin 내 free 리스트
    struct block *fprev;    // bin 내 free 리스트
} block_t;

static block_t *heap_head = NULL;
static pthread_mutex_t heap_lock = PTHREAD_MUTEX_INITIALIZER;

/* ---- size→free-list bins (해시 버킷) ----
 * 같은 payload size를 가진 free block을 bins[idx]에 보관.
 * 요청 사이즈와 "정확히 동일"한 free block이 있으면 즉시 재사용.
 */
#define NBINS 257
static block_t *bins[NBINS];

static inline size_t bin_idx(size_t sz) { return sz % NBINS; }

static void bin_insert(block_t *b) {
    size_t i = bin_idx(b->size);
    b->fprev = NULL;
    b->fnext = bins[i];
    if (bins[i]) bins[i]->fprev = b;
    bins[i] = b;
}

static void bin_remove(block_t *b) {
    size_t i = bin_idx(b->size);
    if (b->fprev) b->fprev->fnext = b->fnext;
    else if (bins[i] == b) bins[i] = b->fnext;
    if (b->fnext) b->fnext->fprev = b->fprev;
    b->fprev = b->fnext = NULL;
}

/* 내부 함수 선언 */
static void *request_from_os(size_t size);
static block_t *find_free_block_firstfit(size_t size);
static void split_block(block_t *block, size_t size);
static void coalesce(block_t *block);

/* ===== 초기화 ===== */
void mh_init(void) {
    if (heap_head) return;
    heap_head = (block_t *)request_from_os(HEAP_SIZE);
    if (!heap_head) {
        fprintf(stderr, "[myheap] mmap failed\n");
        return;
    }
    heap_head->size = HEAP_SIZE - sizeof(block_t);
    heap_head->free = 1;
    heap_head->next = NULL;
    heap_head->prev = NULL;
    heap_head->fnext = heap_head->fprev = NULL;
    bin_insert(heap_head);  // 초기 free 블록을 bin에 넣음
}

/* mmap 요청 */
static void *request_from_os(size_t size) {
    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (p == MAP_FAILED) ? NULL : p;
}

/* first-fit: 힙 순서로 순회하여 충분히 큰 free 블록 하나 찾기 */
static block_t *find_free_block_firstfit(size_t size) {
    block_t *cur = heap_head;
    while (cur) {
        if (cur->free && cur->size >= size) return cur;
        cur = cur->next;
    }
    return NULL;
}

/* 큰 free 블록을 요청 크기로 나누기(split)
 * - block은 사용 블록이 될 것.
 * - 남는 조각은 free 블록으로 bins에 넣는다.
 */
static void split_block(block_t *block, size_t size) {
    if (block->size <= size + sizeof(block_t)) return;  // 남는 조각이 너무 작으면 분할 X

    block_t *newb = (block_t *)((char *)block + sizeof(block_t) + size);
    newb->size = block->size - size - sizeof(block_t);
    newb->free = 1;
    newb->prev = block;
    newb->next = block->next;
    newb->fnext = newb->fprev = NULL;

    if (newb->next) newb->next->prev = newb;
    block->next = newb;
    block->size = size;

    bin_insert(newb); // 새 free 조각을 bin에 등록
}

/* free 후 앞/뒤가 free면 병합
 * 병합에 참여하는 기존 free 블록들은 bin에서 제거하고,
 * 병합 결과 블록을 bin에 다시 넣는다.
 */
static void coalesce(block_t *b) {
    // 뒤쪽 병합
    if (b->next && b->next->free) {
        block_t *n = b->next;
        bin_remove(n); // 기존 free 조각 제거
        b->size += sizeof(block_t) + n->size;
        b->next = n->next;
        if (b->next) b->next->prev = b;
    }
    // 앞쪽 병합
    if (b->prev && b->prev->free) {
        block_t *p = b->prev;
        bin_remove(p); // 기존 free 조각 제거
        p->size += sizeof(block_t) + b->size;
        p->next = b->next;
        if (p->next) p->next->prev = p;
        b = p;
    }
    // 최종 병합 결과를 bin에 보관
    bin_insert(b);
}

/* ====== public API ====== */

void *mh_malloc(size_t size) {
    if (size == 0) return NULL;
    pthread_mutex_lock(&heap_lock);
    size = ALIGN_UP(size);
    if (!heap_head) mh_init();

    /* 1) 정확히 같은 크기의 free 청크가 bins에 있으면 즉시 사용 (요구사항 5) */
    size_t i = bin_idx(size);
    block_t *c = bins[i];
    while (c && !(c->free && c->size == size)) c = c->fnext;
    if (c) {
        // bin에서 제거하고 사용 표시
        bin_remove(c);
        c->free = 0;
        c->fnext = c->fprev = NULL;
        pthread_mutex_unlock(&heap_lock);
        return (char *)c + sizeof(block_t);
    }

    /* 2) 없으면 first-fit으로 충분히 큰 free 블록 찾기 */
    block_t *blk = find_free_block_firstfit(size);
    if (!blk) {
        pthread_mutex_unlock(&heap_lock);
        return NULL; // (확장 아레나 구현 안 함; 필요하면 여기서 mmap 추가 확보)
    }

    /* 선택된 free 블록은 bins에 있으므로 먼저 제거하고, 필요시 split */
    bin_remove(blk);
    split_block(blk, size);
    blk->free = 0;
    blk->fnext = blk->fprev = NULL;

    pthread_mutex_unlock(&heap_lock);
    return (char *)blk + sizeof(block_t);
}

void mh_free(void *ptr) {
    if (!ptr) return;
    pthread_mutex_lock(&heap_lock);

    block_t *b = (block_t *)((char *)ptr - sizeof(block_t));
    if (b->free) { // 간단 이중해제 가드
        pthread_mutex_unlock(&heap_lock);
        fprintf(stderr, "[myheap] double free?\n");
        return;
    }
    b->free = 1;
    b->fnext = b->fprev = NULL;

    /* 병합(coalesce) 수행(내부에서 bin 갱신) */
    coalesce(b);

    pthread_mutex_unlock(&heap_lock);
}

void *mh_realloc(void *ptr, size_t new_size) {
    if (!ptr) return mh_malloc(new_size);
    if (new_size == 0) { mh_free(ptr); return NULL; }

    pthread_mutex_lock(&heap_lock);
    new_size = ALIGN_UP(new_size);
    block_t *b = (block_t *)((char *)ptr - sizeof(block_t));

    if (b->size >= new_size) {
        /* 남는 공간이 충분하면 선택적으로 split 가능 */
        split_block(b, new_size);
        pthread_mutex_unlock(&heap_lock);
        return ptr;
    }
    pthread_mutex_unlock(&heap_lock);

    /* 새로 할당 → 복사 → 기존 free */
    void *np = mh_malloc(new_size);
    if (!np) return NULL;
    memcpy(np, ptr, b->size);
    mh_free(ptr);
    return np;
}

void mh_dump_stats(void) {
    pthread_mutex_lock(&heap_lock);
    size_t used = 0, freec = 0;
    block_t *cur = heap_head;
    while (cur) {
        if (cur->free) freec++; else used++;
        cur = cur->next;
    }
    printf("[myheap] used=%zu free=%zu\n", used, freec);
    pthread_mutex_unlock(&heap_lock);
}
