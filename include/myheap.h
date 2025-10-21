#ifndef MYHEAP_H
#define MYHEAP_H

#include <stddef.h>  // size_t

#ifdef __cplusplus
extern "C" {
#endif

/* === public API (표준 malloc/free와 충돌 피하기 위해 접두사 mh_) === */
/* 힙 초기화(선택사항; 자동 lazy init도 가능하지만 테스트 편의를 위해 둠) */
void mh_init(void);

/* 동적 메모리 할당/해제/재할당 */
void* mh_malloc(size_t size);
void  mh_free(void* ptr);
void* mh_realloc(void* ptr, size_t new_size);

/* (선택) 정렬 할당: 특정 바이트 경계로 정렬된 블록이 필요할 때 */
void* mh_memalign(size_t alignment, size_t size);

/* 디버그용: 내부 상태 출력(예: free list 길이 등) */
void  mh_dump_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* MYHEAP_H */
