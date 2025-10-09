#include <stdio.h>
#include <string.h>
#include "myheap.h"

int main(void) {
    mh_init();               // 초기화 (lazy init도 되지만 명시적으로 해봄)

    void *a = mh_malloc(24);
    void *b = mh_malloc(40);
    printf("a=%p b=%p\n", a, b);

    strcpy((char*)a, "hello myheap");
    printf("a says: %s\n", (char*)a);

    mh_dump_stats();         // 현재 사용/빈 블록 개수

    mh_free(b);
    mh_free(a);

    mh_dump_stats();         // free 후 상태 확인
    return 0;
}
