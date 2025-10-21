#include <stdio.h>
#include <string.h>
#include "myheap.h"

int main() {
    mh_init();
    void *a = mh_malloc(40);
    void *b = mh_malloc(24);
    mh_free(a);                 // 40B 블록을 free
    void *c = mh_malloc(40);    // 같은 40B 요청 → a 자리(또는 동일 size 블록) 재사용
    printf("a=%p b=%p c=%p\n", a,b,c);
    printf("c == a ? %s\n", (c==a) ? "YES(exact reuse)" : "NO");
    return 0;
}
