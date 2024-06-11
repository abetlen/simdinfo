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
// simdinfo is a header-only library for detecting SIMD instruction set support
// at runtime and compile time. It can be used to build programs for specific
// architectures like x86_64 that can take advantage of multiple instruction
// sets depending on the machine it is running on.
//
// The library exposes a struct `simdinfo_t` that contains flags for various
// SIMD instruction sets.
//
// There are two functions exposed by the library:
// - `simdinfo_static` returns a `simdinfo_t` struct with compile-time
// information about SIMD support.
// - `simdinfo_runtime` returns a `simdinfo_t` struct with runtime
// information about SIMD support.
//

#ifndef _SIMDINFO_H_
#define _SIMDINFO_H_

#if defined(__aarch64__)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#endif // __aarch64__

struct simdinfo_t {
  // Check for AVX
  // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L106
  unsigned supports_avx;
  // Check for AVX2 (Function ID 7, EBX register)
  // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L148
  unsigned supports_avx2;
  // Check for F16C (Function ID 1, ECX register)
  // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L107
  unsigned supports_f16c;
  unsigned supports_fma;
  // Check for AVX512F (Function ID 7, EBX register)
  // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L155
  unsigned supports_avx512f;
  // Check for AVX512FP16 (Function ID 7, EDX register)
  // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L198C9-L198C23
  unsigned supports_avx512fp16;
  // Check for AVX512VNNI (Function ID 7, ECX register)
  unsigned supports_avx512vnni;
  // Check for AVX512IFMA (Function ID 7, EBX register)
  unsigned supports_avx512ifma;
  // Check for AVX512BITALG (Function ID 7, ECX register)
  unsigned supports_avx512bitalg;
  // Check for AVX512VBMI2 (Function ID 7, ECX register)
  unsigned supports_avx512vbmi2;
  // Check for AVX512VPOPCNTDQ (Function ID 7, ECX register)
  unsigned supports_avx512vpopcntdq;

  unsigned supports_neon;
  // https://github.com/torvalds/linux/blob/83a7eefedc9b56fe7bfeff13b6c7356688ffa670/arch/arm64/include/uapi/asm/hwcap.h#L48
  unsigned supports_sve;
  // https://github.com/torvalds/linux/blob/83a7eefedc9b56fe7bfeff13b6c7356688ffa670/arch/arm64/include/uapi/asm/hwcap.h#L63
  unsigned supports_sve2;
};

static inline struct simdinfo_t simdinfo_static() {
  return (struct simdinfo_t) {
#if defined(__AVX512F__)
    .supports_avx512f = 1,
#endif // __AVX512F__
#if defined(__AVX512FP16__)
    .supports_avx512fp16 = 1,
#endif // __AVX512FP16__
#if defined(__AVX512VNNI__)
    .supports_avx512vnni = 1,
#endif // __AVX512VNNI__
#if defined(__AVX512IFMA__)
    .supports_avx512ifma = 1,
#endif // __AVX512IFMA__
#if defined(__AVX512BITALG__)
    .supports_avx512bitalg = 1,
#endif // __AVX512BITALG__
#if defined(__AVX512VBMI2__)
    .supports_avx512vbmi2 = 1,
#endif // __AVX512VBMI2__
#if defined(__AVX512VPOPCNTDQ__)
    .supports_avx512vpopcntdq = 1,
#endif // __AVX512VPOPCNTDQ__
#if defined(__AVX__)
    .supports_avx = 1,
#endif // __AVX__
#if defined(__AVX2__)
    .supports_avx2 = 1,
#endif // __AVX2__
#if defined(__ARM_FEATURE_SVE)
    .supports_sve = 1,
#endif // __ARM_FEATURE_SVE
#if defined(__ARM_NEON)
    .supports_neon = 1,
#endif // __ARM_NEON
  };
}

static inline struct simdinfo_t simdinfo_runtime_internal() {
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
  // Check for AVX
  // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L106
  info.supports_avx = (info7.named.ecx & 0x10000000) != 0;
  // Check for AVX2 (Function ID 7, EBX register)
  // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L148
  info.supports_avx2 = (info7.named.ebx & 0x00000020) != 0;
  // Check for F16C (Function ID 1, ECX register)
  // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L107
  info.supports_f16c = (info1.named.ecx & 0x20000000) != 0;
  info.supports_fma = (info1.named.ecx & 0x00001000) != 0;
  // Check for AVX512F (Function ID 7, EBX register)
  // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L155
  info.supports_avx512f = (info7.named.ebx & 0x00010000) != 0;
  // Check for AVX512FP16 (Function ID 7, EDX register)
  // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L198C9-L198C23
  info.supports_avx512fp16 = (info7.named.edx & 0x00800000) != 0;
  // Check for AVX512VNNI (Function ID 7, ECX register)
  info.supports_avx512vnni = (info7.named.ecx & 0x00000800) != 0;
  // Check for AVX512IFMA (Function ID 7, EBX register)
  info.supports_avx512ifma = (info7.named.ebx & 0x00200000) != 0;
  // Check for AVX512BITALG (Function ID 7, ECX register)
  info.supports_avx512bitalg = (info7.named.ecx & 0x00001000) != 0;
  // Check for AVX512VBMI2 (Function ID 7, ECX register)
  info.supports_avx512vbmi2 = (info7.named.ecx & 0x00000040) != 0;
  // Check for AVX512VPOPCNTDQ (Function ID 7, ECX register)
  info.supports_avx512vpopcntdq = (info7.named.ecx & 0x00004000) != 0;
  return info;

#endif // __x86_64__ || _M_X64 || __i386 || _M_IX86

#if defined(__aarch64__)

  // Every 64-bit Arm CPU supports NEON
  info.supports_neon = 1;
  // https://github.com/torvalds/linux/blob/83a7eefedc9b56fe7bfeff13b6c7356688ffa670/arch/arm64/include/uapi/asm/hwcap.h#L48
  info.supports_sve = 0;
  // https://github.com/torvalds/linux/blob/83a7eefedc9b56fe7bfeff13b6c7356688ffa670/arch/arm64/include/uapi/asm/hwcap.h#L63
  info.supports_sve2 = 0;

#ifdef __linux__
  unsigned long hwcap = getauxval(AT_HWCAP);
  unsigned long hwcap2 = getauxval(AT_HWCAP2);
  info.supports_sve = (hwcap & HWCAP_SVE) != 0;
  info.supports_sve2 = (hwcap2 & HWCAP2_SVE2) != 0;
#endif // __linux__
  return info;
#endif // __aarch64__
  return info;
}

static inline struct simdinfo_t simdinfo_runtime() {
#if __GNUC__
  static __thread unsigned initialized = 0;
  static __thread struct simdinfo_t info = {0};
  if (__builtin_expect(initialized, 1)) {
    return info;
  }
  info = simdinfo_runtime_internal();
  initialized = 1;
  return info;
#elif MSC_VER
  static __declspec(thread) unsigned initialized = 0;
  static __declspec(thread) struct simdinfo_t info = {0};
  if (initialized) {
    return info;
  }
  info = simdinfo_get_info_runtime_internal();
  initialized = 1;
  return info;
#else
  return simdinfo_get_info_runtime_internal();
#endif
}

#endif // _SIMDINFO_H_