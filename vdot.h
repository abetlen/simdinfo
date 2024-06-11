/* Fast dot product of two vectors */
#ifndef VDOT_H
#define VDOT_H

#include <stdlib.h>
#include "simdinfo.h"

/* Fallback scalar implementation */

// static inline float _vdot_f32_scalar(float *a, float *b, size_t size) {
//   float sum = 0.0;
//   for (size_t i = 0; i < size; i++) {
//     sum += a[i] * b[i];
//   }
//   return sum;
// }

static inline float _vdot_f32_serial(float *a, float *b, size_t size) {
  // Use Kahan summation algorithm to reduce floating point errors
  // https://en.wikipedia.org/wiki/Kahan_summation_algorithm
  float sum = 0.0;
  float c = 0.0;
  for (size_t i = 0; i < size; i++) {
    float y = a[i] * b[i] - c;
    float t = sum + y;
    c = (t - sum) - y;
    sum = t;
  }
  return sum;
}

#if defined(__AVX__) || defined(__AVX2__)

// Include the necessary headers for SIMD intrinsics
#include <immintrin.h>
#include <smmintrin.h>

// static inline float _vdot_f32_avx(float *a, float *b, size_t size) {
//   __m256 sum = _mm256_setzero_ps();
//   size_t ssize = size - (size % 8);
//   for (size_t i = 0; i < ssize; i += 8) {
//     __m256 va = _mm256_loadu_ps(a + i);
//     __m256 vb = _mm256_loadu_ps(b + i);
//     sum = _mm256_add_ps(sum, _mm256_mul_ps(va, vb));
//   }
//   float result[8];
//   _mm256_storeu_ps(result, sum);
//   float x = result[0] + result[1] + result[2] + result[3] + result[4] +
//             result[5] + result[6] + result[7];
//   // left over
//   for (size_t i = ssize; i < size; i++) {
//     x += a[i] * b[i];
//   }
//   return x;
// }

static inline float _vdot_f32_avx(float *a, float *b, size_t size) {
  __m256 sum = _mm256_setzero_ps();
  __m256 cvec = _mm256_setzero_ps();
  size_t ssize = size - (size % 8);
  for (size_t i = 0; i < ssize; i += 8) {
    __m256 va = _mm256_loadu_ps(a + i);
    __m256 vb = _mm256_loadu_ps(b + i);
    __m256 y = _mm256_sub_ps(_mm256_mul_ps(va, vb), cvec);
    __m256 t = _mm256_add_ps(sum, y);
    cvec = _mm256_sub_ps(_mm256_sub_ps(t, sum), y);
    sum = t;
  }
  float result[8];
  _mm256_storeu_ps(result, sum);
  float x = result[0] + result[1] + result[2] + result[3] + result[4] +
            result[5] + result[6] + result[7];
  _mm256_storeu_ps(result, cvec);
  // left over
  float c = result[0] + result[1] + result[2] + result[3] + result[4] +
            result[5] + result[6] + result[7];
  for (size_t i = ssize; i < size; i++) {
    float y = a[i] * b[i] - c;
    float t = x + y;
    c = (t - x) - y;
    x = t;
  }
  return x;
}

#endif // __AVX__ || __AVX2__

#if defined(__AVX512F__)

#include <immintrin.h>

static inline float vdot_avx512f(float *a, float *b, size_t size) {
  __m512 va, vb, vsum = _mm512_setzero_ps();

  size_t i;
  for (i = 0; i < size; i += 16) {
    va = _mm512_load_ps(&a[i]);
    vb = _mm512_load_ps(&b[i]);
    vsum = _mm512_fmadd_ps(va, vb, vsum);
  }

  float sum[8];
  _mm512_storeu_ps(sum, vsum);

  float dot_product = 0.0f;
  for (i = 0; i < 8; i++) {
    dot_product += sum[i];
  }

  return dot_product;
}

#endif // __AVX512F__

/* ARM */

#if defined(__ARM_FEATURE_SVE)

#include <arm_sve.h>

static inline float vdot_sve(float *a, float *b, size_t size) {
    // Initialize the dot product result
    float result = 0.0f;
    // Create a zeroed float vector
    svfloat32_t sum_vec = svdup_f32(0.0f);

    // Process the vectors in chunks using SVE
    size_t i = 0;
    size_t vec_size = svcntw(); // Vector size in terms of number of float32 elements
    while (i + vec_size <= size) {
        // Load chunks of the vectors
        svbool_t pg = svwhilelt_b32(i, size);
        svfloat32_t a_vec = svld1_f32(pg, &a[i]);
        svfloat32_t b_vec = svld1_f32(pg, &b[i]);
        
        // Perform element-wise multiplication and accumulate
        sum_vec = svmla_f32_m(pg, sum_vec, a_vec, b_vec);
        
        i += vec_size;
    }

    // Handle any remaining elements
    if (i < size) {
        svbool_t pg = svwhilelt_b32(i, size);
        svfloat32_t a_vec = svld1_f32(pg, &a[i]);
        svfloat32_t b_vec = svld1_f32(pg, &b[i]);
        sum_vec = svmla_f32_m(pg, sum_vec, a_vec, b_vec);
    }

    // Sum up the elements in the result vector
    result = svaddv_f32(svptrue_b32(), sum_vec);

    return result;
}

#endif // __ARM_FEATURE_SVE

#if defined(__ARM_NEON)

#include <arm_neon.h>

// static inline float _vdot_f32_neon(float *a, float *b, size_t size) {
//   float32x4_t sum = vdupq_n_f32(0);
//   size_t ssize = size - (size % 4);
//   for (size_t i = 0; i < ssize; i += 4) {
//     float32x4_t va = vld1q_f32(a + i);
//     float32x4_t vb = vld1q_f32(b + i);
//     sum = vmlaq_f32(sum, va, vb);
//   }
//   float32_t result[4];
//   vst1q_f32(result, sum);
//   float x = result[0] + result[1] + result[2] + result[3];
//   // left over
//   for (size_t i = ssize; i < size; i++) {
//     x += a[i] * b[i];
//   }
//   return x;
// }

static inline float _vdot_f32_neon(float *a, float *b, size_t size) {
  float32x4_t sum = vdupq_n_f32(0);
  float32x4_t cvec = vdupq_n_f32(0);
  size_t ssize = size - (size % 4);
  for (size_t i = 0; i < ssize; i += 4) {
    float32x4_t va = vld1q_f32(a + i);
    float32x4_t vb = vld1q_f32(b + i);
    float32x4_t y = vsubq_f32(vmulq_f32(va, vb), cvec);
    float32x4_t t = vaddq_f32(sum, y);
    cvec = vsubq_f32(vsubq_f32(t, sum), y);
    sum = t;
  }
  float32_t result[4];
  vst1q_f32(result, sum);
  float x = result[0] + result[1] + result[2] + result[3];
  vst1q_f32(result, cvec);
  // left over
  float c = result[0] + result[1] + result[2] + result[3];
  for (size_t i = ssize; i < size; i++) {
    float y = a[i] * b[i] - c;
    float t = x + y;
    c = (t - x) - y;
    x = t;
  }
  return x;
}

#endif // __ARM_NEON

#ifndef VDOT_STATIC_DISPATCH
#define VDOT_STATIC_DISPATCH 1
#endif // VDOT_STATIC_DISPATCH

float vdot_f32(float *a, float *b, size_t size) {
#if VDOT_STATIC_DISPATCH
  struct simdinfo_t info = simdinfo_static();
#else
  struct simdinfo_t info = simdinfo_runtime();
#endif

// x86
#if defined(__AVX512F__)
  if (VDOT_STATIC_DISPATCH || info.supports_avx512f) {
    return vdot_avx512f(a, b, size);
  }
#endif // __AVX512F__
#if defined(__AVX__) || defined(__AVX2__)
  if (VDOT_STATIC_DISPATCH || info.supports_avx || info.supports_avx2) {
    return _vdot_f32_avx(a, b, size);
  }
#endif // __AVX__ || __AVX2__

// ARM
#if defined(__ARM_FEATURE_SVE)
  if (VDOT_STATIC_DISPATCH || info.supports_sve) {
    return vdot_sve(a, b, size);
  }
#endif // __ARM_FEATURE_SVE
#if defined(__ARM_NEON)
  if (VDOT_STATIC_DISPATCH || info.supports_neon) {
    return _vdot_f32_neon(a, b, size);
  }
#endif // __ARM_NEON

  // Default
  // Fall back to serial implementation
  return _vdot_f32_serial(a, b, size);
}

#endif // VDOT_H