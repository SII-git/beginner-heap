#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "myheap.h"

#define N      2000     // 동시에 유지할 최대 포인터 개수
#define ITERS  200000   // 반복 횟수
#define MAXSZ  1024     // 최대 할당 크기(바이트)

static void touch(void *p, size_t n) {
    if (!p || n==0) return;
    unsigned char *c = (unsigned char*)p;
    c[0] = 0xAA;
    c[n-1] = 0x55; // 가장자리도 가볍게 터치
}

int main(void) {
    srand((unsigned)time(NULL));
    mh_init();

    void *arr[N] = {0};
    size_t sz[N] = {0};

    for (int i=0; i<ITERS; i++) {
        int idx = rand() % N;
        if (arr[idx]) {
            // 가끔은 realloc도 테스트
            if ((rand() & 7) == 0) {
                size_t newsz = (rand() % MAXSZ) + 1;
                void *np = mh_realloc(arr[idx], newsz);
                if (!np) {
                    // realloc 실패 시 기존 포인터는 여전히 유효 — 그냥 유지
                } else {
                    arr[idx] = np;
                    sz[idx] = newsz;
                    touch(arr[idx], sz[idx]);
                }
            } else {
                mh_free(arr[idx]);
                arr[idx] = NULL;
                sz[idx] = 0;
            }
        } else {
            size_t s = (rand() % MAXSZ) + 1;
            void *p = mh_malloc(s);
            if (p) {
                arr[idx] = p; sz[idx] = s;
                touch(arr[idx], sz[idx]);
            }
        }

        // 가끔 통계 찍기
        if ((i % 50000) == 0) {
            mh_dump_stats();
        }
    }

    // 남은 것들 정리
    for (int i=0; i<N; i++) {
        if (arr[i]) mh_free(arr[i]);
    }

    mh_dump_stats();
    puts("[stress] done");
    return 0;
}
