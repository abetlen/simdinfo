# simdinfo

simdinfo is a header-only library for detecting SIMD support at runtime.

Say you have a function that already checks for SIMD support at compile time

```c
float sum(float *a, size_t size) {
#if defined(__AVX__) || defined(__AVX2__)
  return sum_avx(a, size);
#else
  return sum_scalar(a, size);
#endif
}
```

You can replace the compile-time check with a runtime check:

```c
#include "simdinfo.h"

float sum(float *a, size_t size) {
  simdinfo_t info = simdinfo();
  if (SIMDINFO_SUPPORTS(info, __AVX__) || SIMDINFO_SUPPORTS(info, __AVX2__)) {
    return sum_avx(a, size);
  } else {
    return sum_scalar(a, size);
  }
}
```

# Basic Example (Sum of Floats)

Let's start with a little example of a c function that sums up an array of floats.

```c
float sum_serial(float *data, size_t n) {
    float sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += data[i];
    }
    return sum;
}
```

This function iterates over a data array one element at a time and adds it to a running sum.

If we want to make this function faster one thing we can do is make use of specialised SIMD instructions to add up multiple elements at the same time.

The following uses x86_64 AVX simd intrinsics to load 8 floats a time into a vector and add them to a sum vector holding 8 partial sums.

```c
#if defined(__AVX__)
#include <immintrin.h>

float sum_avx(float *data, size_t n) {
    __m256 sum = _mm256_setzero_ps();
    for (size_t i = 0; i < n; i += 8) {
        __m256 v = _mm256_loadu_ps(data + i);
        sum = _mm256_add_ps(sum, v);
    }
    float result[8];
    _mm256_storeu_ps(result, sum);
    float x = result[0] + result[1] + result[2] + result[3] + result[4] + result[5] + result[6] + result[7];
    // handle remaining elements since n is not always divisible by 8
    for (size_t i = n - n % 8; i < n; i++) {
        x += data[i];
    }
    return x;
}
#endif
```

The above code will be compiled only if the compiler supports AVX instructions. Usually this is done by setting `-march=native` if you're compiling the code for the machine you're running it on.

What do you do if don't know if the machine you're running the code on supports AVX instructions? You can manually tell the compiler to include the AVX code path by setting `-mavx` flag and use `SIMDINFO_SUPPORTS` to check if the machine supports AVX instructions at runtime.

Below is a full example

```c
// sum.c
#include <stdio.h>
#include <stdlib.h>
#include "simdinfo.h"

float sum_serial(float *data, size_t n) {
    float sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += data[i];
    }
    return sum;
}

#if defined(__AVX__)

#include <immintrin.h>

float sum_avx(float *data, size_t n) {
    __m256 sum = _mm256_setzero_ps();
    for (size_t i = 0; i < n; i += 8) {
        __m256 v = _mm256_loadu_ps(data + i);
        sum = _mm256_add_ps(sum, v);
    }
    float result[8];
    _mm256_storeu_ps(result, sum);
    float x = result[0] + result[1] + result[2] + result[3] + result[4] + result[5] + result[6] + result[7];
    for (size_t i = n - n % 8; i < n; i++) {
        x += data[i];
    }
    return x;
}
#endif

float sum(float *data, size_t n) {
    simdinfo_t info = simdinfo();
#if defined(__AVX__)
    if (SIMDINFO_SUPPORTS(info, __AVX__)) {
        return sum_avx(data, n);
    }
#endif
    return sum_serial(data, n);
}

int main() {
    size_t n = 1000000;
    float *data = malloc(n * sizeof(float));
    for (size_t i = 0; i < n; i++) {
        data[i] = 1.0;
    }
    float s = sum(data, n);
    printf("sum = %f\n", s);
    free(data);
    return 0;
}
```

Then we can compile the code for a generic `x86_64` machine with `-mavx` flags.

```bash
wget https://raw.githubusercontent.com/abetlen/simdinfo/main/simdinfo.h
gcc \
    -march=x86_64 \
    -mavx \
    -O2 \
    -mtune=generic \
    -o sum \
    sum.c \
    -I.
./sum
```

# Related Projects

- https://github.com/ashvardanian/SimSIMD
- https://github.com/Mozilla-Ocho/llamafile
- https://github.com/google/cpu_features
- https://github.com/google/highway
