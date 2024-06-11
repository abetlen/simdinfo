#include <stdio.h>
#include <stdlib.h>
#include "vdot.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <size>\n", argv[0]);
        return 1;
    }
    int size = atoi(argv[1]);
    if (size <= 0) {
        printf("Invalid size\n");
        return 1;
    }
    size_t n = (size_t)size;
    float * a = (float *)malloc(n * sizeof(float));
    float * b = (float *)malloc(n * sizeof(float));

    if (a == NULL || b == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }

    for (size_t i = 0; i < n; i++) {
        a[i] = i;
        b[i] = i;
    }

    // 0*0 + 1*1 + 2*2 + ... + (n-1)*(n-1) = n*(n-1)*(2n-1)/6
    float result = vdot_f32(a, b, n);
    printf("Result: %.2f\n", result);
    return 0;
}