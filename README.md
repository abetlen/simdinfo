# simdinfo

simdinfo is a header-only library for detecting supported SIMD instruction set support at runtime.
It's primary use is for dynamic dispatching of SIMD code paths based on the available instruction sets on the target machine.
For example you can compile a generic binary application or library for x86_64 linux machines that can detect and use SSE2, AVX, AVX2, AVX512, etc. depending on what's available on the target machine at runtime

This is ideal for pre-built binaries such as python wheels or system packages where you can't compile for every possible target machine.

# Usage

```c
#include "simdinfo.h"

#ifdef __AVX512F__
float sum_avx512f(float *data, size_t n) {
    // AVX512 code path
}
#endif

#ifdef __AVX__
float sum_avx(float *data, size_t n) {
    // AVX2 code path
}
#endif

float sum_serial(float *data, size_t n) {
    // Serial code path
}

float sum(float *data, size_t n) {
    struct simdinfo_t info = simdinfo_runtime();
#if defined(__AVX512F__)
    if (info.supports_avx512f) {
        return sum_avx512f(data, n);
    }
#endif
#if defined(__AVX__)
    if (info.supports_avx) {
        return sum_avx(data, n);
    }
#endif
    return sum_serial(data, n);
}
```

Then we can compile the code for a generic `x86_64` machine with `-mavx -mavx2 -mavx512f` flags.

```bash
gcc -mavx -mavx2 -mavx512f -O2 -march=x86_64 -mtune=generic -o simdinfo_example simdinfo_example.c -Ipath/to/simdinfo
./simdinfo_example
```

# Related Projects

- https://github.com/ashvardanian/SimSIMD
- https://github.com/Mozilla-Ocho/llamafile
- https://github.com/google/highway
