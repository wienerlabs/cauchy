/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * Platform Detection and Configuration
 * 
 * Detects architecture, compiler, and OS to configure optimal code paths
 * for atomic operations and memory ordering.
 */

#ifndef CAUCHY_PLATFORM_H
#define CAUCHY_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Architecture detection */
#if defined(__x86_64__) || defined(_M_X64)
    #define CAUCHY_ARCH_X86_64 1
    #define CAUCHY_CACHE_LINE_SIZE 64
    #define CAUCHY_HAS_DWCAS 1  /* Double-width CAS via CMPXCHG16B */
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define CAUCHY_ARCH_ARM64 1
    #define CAUCHY_CACHE_LINE_SIZE 64
    #if defined(__ARM_FEATURE_ATOMICS)
        #define CAUCHY_HAS_LSE 1  /* Large System Extensions */
    #endif
    #define CAUCHY_HAS_DWCAS 1  /* LDXP/STXP pair */
#elif defined(__arm__) || defined(_M_ARM)
    #define CAUCHY_ARCH_ARM32 1
    #define CAUCHY_CACHE_LINE_SIZE 32
    #define CAUCHY_HAS_DWCAS 0
#else
    #define CAUCHY_ARCH_GENERIC 1
    #define CAUCHY_CACHE_LINE_SIZE 64
    #define CAUCHY_HAS_DWCAS 0
#endif

/* Compiler detection */
#if defined(__GNUC__) && !defined(__clang__)
    #define CAUCHY_COMPILER_GCC 1
    #define CAUCHY_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)
#elif defined(__clang__)
    #define CAUCHY_COMPILER_CLANG 1
    #define CAUCHY_CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100)
#elif defined(_MSC_VER)
    #define CAUCHY_COMPILER_MSVC 1
#else
    #define CAUCHY_COMPILER_UNKNOWN 1
#endif

/* OS detection */
#if defined(__linux__)
    #define CAUCHY_OS_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
    #define CAUCHY_OS_MACOS 1
#elif defined(_WIN32) || defined(_WIN64)
    #define CAUCHY_OS_WINDOWS 1
#elif defined(__FreeBSD__)
    #define CAUCHY_OS_FREEBSD 1
#else
    #define CAUCHY_OS_UNKNOWN 1
#endif

/* Compiler attributes */
#if defined(CAUCHY_COMPILER_GCC) || defined(CAUCHY_COMPILER_CLANG)
    #define CAUCHY_ALIGNED(n)     __attribute__((aligned(n)))
    #define CAUCHY_PACKED         __attribute__((packed))
    #define CAUCHY_LIKELY(x)      __builtin_expect(!!(x), 1)
    #define CAUCHY_UNLIKELY(x)    __builtin_expect(!!(x), 0)
    #define CAUCHY_INLINE         static inline __attribute__((always_inline))
    #define CAUCHY_NOINLINE       __attribute__((noinline))
    #define CAUCHY_NORETURN       __attribute__((noreturn))
    #define CAUCHY_UNUSED         __attribute__((unused))
    #define CAUCHY_PREFETCH(addr) __builtin_prefetch(addr)
    #define CAUCHY_RESTRICT       __restrict__
#elif defined(CAUCHY_COMPILER_MSVC)
    #define CAUCHY_ALIGNED(n)     __declspec(align(n))
    #define CAUCHY_PACKED
    #define CAUCHY_LIKELY(x)      (x)
    #define CAUCHY_UNLIKELY(x)    (x)
    #define CAUCHY_INLINE         static __forceinline
    #define CAUCHY_NOINLINE       __declspec(noinline)
    #define CAUCHY_NORETURN       __declspec(noreturn)
    #define CAUCHY_UNUSED
    #define CAUCHY_PREFETCH(addr)
    #define CAUCHY_RESTRICT       __restrict
#else
    #define CAUCHY_ALIGNED(n)
    #define CAUCHY_PACKED
    #define CAUCHY_LIKELY(x)      (x)
    #define CAUCHY_UNLIKELY(x)    (x)
    #define CAUCHY_INLINE         static inline
    #define CAUCHY_NOINLINE
    #define CAUCHY_NORETURN
    #define CAUCHY_UNUSED
    #define CAUCHY_PREFETCH(addr)
    #define CAUCHY_RESTRICT
#endif

/* Cache line alignment macro for struct members */
#define CAUCHY_CACHE_ALIGNED CAUCHY_ALIGNED(CAUCHY_CACHE_LINE_SIZE)

/* Memory fence intrinsics */
#if defined(CAUCHY_COMPILER_GCC) || defined(CAUCHY_COMPILER_CLANG)
    #define CAUCHY_COMPILER_BARRIER() __asm__ __volatile__("" ::: "memory")
    #if defined(CAUCHY_ARCH_X86_64)
        #define CAUCHY_CPU_PAUSE() __asm__ __volatile__("pause" ::: "memory")
    #elif defined(CAUCHY_ARCH_ARM64)
        #define CAUCHY_CPU_PAUSE() __asm__ __volatile__("yield" ::: "memory")
    #else
        #define CAUCHY_CPU_PAUSE() CAUCHY_COMPILER_BARRIER()
    #endif
#elif defined(CAUCHY_COMPILER_MSVC)
    #include <intrin.h>
    #define CAUCHY_COMPILER_BARRIER() _ReadWriteBarrier()
    #define CAUCHY_CPU_PAUSE()        _mm_pause()
#else
    #define CAUCHY_COMPILER_BARRIER()
    #define CAUCHY_CPU_PAUSE()
#endif

/* Version info */
#define CAUCHY_VERSION_MAJOR 0
#define CAUCHY_VERSION_MINOR 1
#define CAUCHY_VERSION_PATCH 0

#ifdef __cplusplus
}
#endif

#endif /* CAUCHY_PLATFORM_H */

