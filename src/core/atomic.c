/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * Platform-Specific Atomic Operations
 */

#include "cauchy/atomic.h"

#if CAUCHY_HAS_DWCAS

#if defined(CAUCHY_ARCH_X86_64)

bool cauchy_atomic_cas_u128(cauchy_atomic_u128_t* ptr,
                            cauchy_u128_t* expected,
                            cauchy_u128_t desired) {
    bool result;
    __asm__ __volatile__(
        "lock cmpxchg16b %1"
        : "=@ccz" (result),
          "+m" (*ptr),
          "+a" (expected->lo),
          "+d" (expected->hi)
        : "b" (desired.lo),
          "c" (desired.hi)
        : "memory", "cc"
    );
    return result;
}

cauchy_u128_t cauchy_atomic_load_u128(const cauchy_atomic_u128_t* ptr) {
    cauchy_u128_t result = {0, 0};
    __asm__ __volatile__(
        "lock cmpxchg16b %1"
        : "+a" (result.lo),
          "+d" (result.hi)
        : "m" (*ptr),
          "b" (result.lo),
          "c" (result.hi)
        : "memory", "cc"
    );
    return result;
}

void cauchy_atomic_store_u128(cauchy_atomic_u128_t* ptr, cauchy_u128_t val) {
    cauchy_u128_t expected = cauchy_atomic_load_u128(ptr);
    while (!cauchy_atomic_cas_u128(ptr, &expected, val)) {
        CAUCHY_CPU_PAUSE();
    }
}

#elif defined(CAUCHY_ARCH_ARM64)

bool cauchy_atomic_cas_u128(cauchy_atomic_u128_t* ptr,
                            cauchy_u128_t* expected,
                            cauchy_u128_t desired) {
    u64 old_lo = expected->lo;
    u64 old_hi = expected->hi;
    u64 new_lo, new_hi;
    u32 result;
    
    __asm__ __volatile__(
        "1: ldaxp %[lo], %[hi], %[ptr]\n"
        "   cmp %[lo], %[old_lo]\n"
        "   ccmp %[hi], %[old_hi], #0, eq\n"
        "   b.ne 2f\n"
        "   stlxp %w[res], %[new_lo], %[new_hi], %[ptr]\n"
        "   cbnz %w[res], 1b\n"
        "   mov %w[res], #1\n"
        "   b 3f\n"
        "2: mov %w[res], #0\n"
        "3:\n"
        : [lo] "=&r" (new_lo),
          [hi] "=&r" (new_hi),
          [res] "=&r" (result),
          [ptr] "+Q" (*ptr)
        : [old_lo] "r" (old_lo),
          [old_hi] "r" (old_hi),
          [new_lo] "r" (desired.lo),
          [new_hi] "r" (desired.hi)
        : "memory", "cc"
    );
    
    if (!result) {
        expected->lo = new_lo;
        expected->hi = new_hi;
    }
    return result;
}

cauchy_u128_t cauchy_atomic_load_u128(const cauchy_atomic_u128_t* ptr) {
    cauchy_u128_t result;
    __asm__ __volatile__(
        "ldaxp %0, %1, %2"
        : "=&r" (result.lo), "=&r" (result.hi)
        : "Q" (*ptr)
        : "memory"
    );
    return result;
}

void cauchy_atomic_store_u128(cauchy_atomic_u128_t* ptr, cauchy_u128_t val) {
    cauchy_u128_t expected = cauchy_atomic_load_u128(ptr);
    while (!cauchy_atomic_cas_u128(ptr, &expected, val)) {
        CAUCHY_CPU_PAUSE();
    }
}

#endif /* Architecture selection */

#endif /* CAUCHY_HAS_DWCAS */

