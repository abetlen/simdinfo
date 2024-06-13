// MIT License
//
// Copyright (c) 2024 Andrei Betlen
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//
// simdinfo.h
//
// simdinfo is a header-only library for detecting SIMD support at runtime.
//
// Say you have a function that already checks for SIMD support at compile time
// 
// ```c
// float sum(float *a, size_t size) {
// #if defined(__AVX__) || defined(__AVX2__)
//   return sum_avx(a, size);
// #else
//   return sum_scalar(a, size);
// #endif
// }
// ```
//
// You can replace the compile-time check with a runtime check:
//
// ```c
// #include "simdinfo.h"
//
// float sum(float *a, size_t size) {
//   simdinfo_t info = simdinfo();
//   if (SIMDINFO_SUPPORTS(info, __AVX__) || SIMDINFO_SUPPORTS(info, __AVX2__)) {
//     return sum_avx(a, size);
//   } else {
//     return sum_scalar(a, size);
//   }
// }
// ```
//

#ifndef _SIMDINFO_H_
#define _SIMDINFO_H_

#if defined(__aarch64__) || defined(__arm__)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#endif // __aarch64__

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif // __APPLE__

typedef struct simdinfo_t {
  // x86 and x86_64
  unsigned _supports__AVX__;
  unsigned _supports__AVX2__;
  unsigned _supports__AVXVNNI__;
  unsigned _supports__F16C__;
  unsigned _supports__FMA__;
  unsigned _supports__AVX512F__;
  unsigned _supports__AVX512BF16__;
  unsigned _supports__AVX512VNNI__;
  unsigned _supports__AVX512VBMI__;
  unsigned _supports__AVX512DQ__;
  unsigned _supports__SSE3__;
  unsigned _supports__SSSE3__;

  // arm and aarch64
  unsigned _supports__ARM_NEON;
  unsigned _supports__ARM_FEATURE_FMA;
  unsigned _supports__ARM_FEATURE_MATMUL_INT8;
  unsigned _supports__ARM_FEATURE_FP16_VECTOR_ARITHMETIC;
  unsigned _supports__ARM_FEATURE_SVE;
  unsigned _supports__ARM_FEATURE_SVE2;
} simdinfo_t;

#define SIMDINFO_SUPPORTS(info, feature) ((info)._supports##feature)

static inline struct simdinfo_t simdinfo_internal() {
  struct simdinfo_t info = {0};
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) ||               \
    defined(_M_IX86)

  /// The states of 4 registers populated for a specific "cpuid" assembly call
  union four_registers_t {
    int array[4];
    struct separate_t {
      unsigned eax, ebx, ecx, edx;
    } named;
  } info1, info7;

#ifdef _MSC_VER
  __cpuidex(info1.array, 1, 0);
  __cpuidex(info7.array, 7, 0);
#else
  __asm__ __volatile__("cpuid"
                       : "=a"(info1.named.eax), "=b"(info1.named.ebx),
                         "=c"(info1.named.ecx), "=d"(info1.named.edx)
                       : "a"(1), "c"(0));
  __asm__ __volatile__("cpuid"
                       : "=a"(info7.named.eax), "=b"(info7.named.ebx),
                         "=c"(info7.named.ecx), "=d"(info7.named.edx)
                       : "a"(7), "c"(0));
#endif
  // source:
  // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h

  info._supports__AVX__ = (info7.named.ecx & 0x10000000) != 0;
  info._supports__AVX2__ = (info7.named.ebx & 0x00000020) != 0;
  info._supports__F16C__ = (info1.named.ecx & 0x20000000) != 0;
  info._supports__FMA__ = (info1.named.ecx & 0x00001000) != 0;
  info._supports__AVXVNNI__ = (info7.named.eax & 0x00000010) != 0;
  info._supports__AVX512F__ = (info7.named.ebx & 0x00010000) != 0;
  info._supports__AVX512BF16__ = (info7.named.edx & 0x00000020) != 0;
  info._supports__AVX512VNNI__ = (info7.named.ecx & 0x00000800) != 0;
  info._supports__AVX512VBMI__ = (info7.named.ecx & 0x00000002) != 0;
  info._supports__AVX512DQ__ = (info7.named.ebx & 0x00020000) != 0;

  return info;

#endif // __x86_64__ || _M_X64 || __i386 || _M_IX86

#if defined(__aarch64__)
  // Every 64-bit Arm CPU supports NEON
  info._supports__ARM_NEON = 1;
  // Every arm version > 7 (all aarch64) supports NEON FMA
  info._supports__ARM_FEATURE_FMA = 1;
  info._supports__ARM_FEATURE_SVE = 0;
  info._supports__ARM_FEATURE_SVE2 = 0;
#ifdef __linux__
  unsigned long hwcap = getauxval(AT_HWCAP);
  unsigned long hwcap2 = getauxval(AT_HWCAP2);
  // https://github.com/torvalds/linux/blob/83a7eefedc9b56fe7bfeff13b6c7356688ffa670/arch/arm64/include/uapi/asm/hwcap.h
  info._supports__ARM_FEATURE_SVE = (hwcap & HWCAP_SVE) != 0;
  info._supports__ARM_FEATURE_SVE2 = (hwcap2 & HWCAP2_SVE2) != 0;
// This is not always defined for some reason
#ifndef HWCAP2_I8MM
#define HWCAP2_I8MM (1 << 13)
#endif
  info._supports__ARM_FEATURE_MATMUL_INT8 = (hwcap2 & HWCAP2_I8MM) != 0;
  info._supports__ARM_FEATURE_FP16_VECTOR_ARITHMETIC =
      (hwcap & HWCAP_ASIMDHP) != 0;
#endif // __linux__
#ifdef __APPLE__
  // use sysctlbyname to get hw.optional.* values
  // https://developer.apple.com/documentation/kernel/1387446-sysctlbyname/determining_instruction_set_characteristics
  size_t size = sizeof(int);
  int hw_optional_arm_feat_i8mm = 0;
  sysctlbyname("hw.optional.arm.FEAT_I8MM", &hw_optional_arm_feat_i8mm, &size,
               NULL, 0);
  info._supports__ARM_FEATURE_MATMUL_INT8 = hw_optional_arm_feat_i8mm;
  int hw_optional_arm_feat_fp16 = 0;
  sysctlbyname("hw.optional.arm.FEAT_FP16", &hw_optional_arm_feat_fp16, &size,
               NULL, 0);
  info._supports__ARM_FEATURE_FP16_VECTOR_ARITHMETIC =
      hw_optional_arm_feat_fp16;
#endif // __APPLE__
  return info;
#endif // __aarch64__

#ifdef __arm__
#ifdef __linux__
  // https://github.com/torvalds/linux/blob/83a7eefedc9b56fe7bfeff13b6c7356688ffa670/arch/arm/include/uapi/asm/hwcap.h
  unsigned long hwcap = getauxval(AT_HWCAP);
  unsigned long hwcap2 = getauxval(AT_HWCAP2);
  info._supports__ARM_NEON = (hwcap & HWCAP_NEON) != 0;
  info._supports__ARM_FEATURE_FMA = (hwcap & HWCAP_VFPv4) != 0;
  info._supports__ARM_FEATURE_MATMUL_INT8 = (hwcap2 & HWCAP_I8MM) != 0;
  info._supports__ARM_FEATURE_FP16_VECTOR_ARITHMETIC =
      (hwcap & HWCAP_ASIMDHP) != 0;
  return info;
#endif // __linux__
#endif // __arm__

  return info;
}

static inline struct simdinfo_t simdinfo() {
  // https://stackoverflow.com/questions/18298280/how-to-declare-a-variable-as-thread-local-portably
#if __GNUC__
  static __thread unsigned initialized = 0;
  static __thread struct simdinfo_t info = {0};
  if (__builtin_expect(initialized, 1)) {
    return info;
  }
  info = simdinfo_internal();
  initialized = 1;
  return info;
#elif MSC_VER
  static __declspec(thread) unsigned initialized = 0;
  static __declspec(thread) struct simdinfo_t info = {0};
  // https://stackoverflow.com/questions/1440570/likely-unlikely-equivalent-for-msvc
  if (initialized) {
    return info;
  }
  info = simdinfo_internal();
  initialized = 1;
  return info;
#else
  return simdinfo_internal();
#endif
}

#endif // _SIMDINFO_H_