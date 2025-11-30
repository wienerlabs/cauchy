/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * Atomic Operations Abstraction
 * 
 * Provides portable atomic primitives with explicit memory ordering.
 * Optimized code paths for x86-64 and ARM64.
 */

#ifndef CAUCHY_ATOMIC_H
#define CAUCHY_ATOMIC_H

#include "types.h"
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Standard atomic types */
typedef _Atomic(u32) cauchy_atomic_u32_t;
typedef _Atomic(u64) cauchy_atomic_u64_t;
typedef _Atomic(void*) cauchy_atomic_ptr_t;
typedef _Atomic(bool) cauchy_atomic_bool_t;

/* Load with acquire semantics */
CAUCHY_INLINE u32 cauchy_atomic_load_u32(const cauchy_atomic_u32_t* ptr) {
    return atomic_load_explicit(ptr, memory_order_acquire);
}

CAUCHY_INLINE u64 cauchy_atomic_load_u64(const cauchy_atomic_u64_t* ptr) {
    return atomic_load_explicit(ptr, memory_order_acquire);
}

CAUCHY_INLINE void* cauchy_atomic_load_ptr(const cauchy_atomic_ptr_t* ptr) {
    return atomic_load_explicit(ptr, memory_order_acquire);
}

/* Store with release semantics */
CAUCHY_INLINE void cauchy_atomic_store_u32(cauchy_atomic_u32_t* ptr, u32 val) {
    atomic_store_explicit(ptr, val, memory_order_release);
}

CAUCHY_INLINE void cauchy_atomic_store_u64(cauchy_atomic_u64_t* ptr, u64 val) {
    atomic_store_explicit(ptr, val, memory_order_release);
}

CAUCHY_INLINE void cauchy_atomic_store_ptr(cauchy_atomic_ptr_t* ptr, void* val) {
    atomic_store_explicit(ptr, val, memory_order_release);
}

/* Compare-and-swap with acquire-release semantics */
CAUCHY_INLINE bool cauchy_atomic_cas_u32(cauchy_atomic_u32_t* ptr, 
                                          u32* expected, u32 desired) {
    return atomic_compare_exchange_strong_explicit(
        ptr, expected, desired,
        memory_order_acq_rel, memory_order_acquire
    );
}

CAUCHY_INLINE bool cauchy_atomic_cas_u64(cauchy_atomic_u64_t* ptr,
                                          u64* expected, u64 desired) {
    return atomic_compare_exchange_strong_explicit(
        ptr, expected, desired,
        memory_order_acq_rel, memory_order_acquire
    );
}

CAUCHY_INLINE bool cauchy_atomic_cas_ptr(cauchy_atomic_ptr_t* ptr,
                                          void** expected, void* desired) {
    return atomic_compare_exchange_strong_explicit(
        ptr, expected, desired,
        memory_order_acq_rel, memory_order_acquire
    );
}

/* Weak CAS for spin loops */
CAUCHY_INLINE bool cauchy_atomic_cas_weak_u64(cauchy_atomic_u64_t* ptr,
                                               u64* expected, u64 desired) {
    return atomic_compare_exchange_weak_explicit(
        ptr, expected, desired,
        memory_order_acq_rel, memory_order_acquire
    );
}

/* Fetch-and-add */
CAUCHY_INLINE u32 cauchy_atomic_fetch_add_u32(cauchy_atomic_u32_t* ptr, u32 val) {
    return atomic_fetch_add_explicit(ptr, val, memory_order_acq_rel);
}

CAUCHY_INLINE u64 cauchy_atomic_fetch_add_u64(cauchy_atomic_u64_t* ptr, u64 val) {
    return atomic_fetch_add_explicit(ptr, val, memory_order_acq_rel);
}

/* Fetch-and-sub */
CAUCHY_INLINE u64 cauchy_atomic_fetch_sub_u64(cauchy_atomic_u64_t* ptr, u64 val) {
    return atomic_fetch_sub_explicit(ptr, val, memory_order_acq_rel);
}

/* Fetch-and-or (useful for flag setting) */
CAUCHY_INLINE u64 cauchy_atomic_fetch_or_u64(cauchy_atomic_u64_t* ptr, u64 val) {
    return atomic_fetch_or_explicit(ptr, val, memory_order_acq_rel);
}

/* Fetch-and-and (useful for flag clearing) */
CAUCHY_INLINE u64 cauchy_atomic_fetch_and_u64(cauchy_atomic_u64_t* ptr, u64 val) {
    return atomic_fetch_and_explicit(ptr, val, memory_order_acq_rel);
}

/* Exchange */
CAUCHY_INLINE u64 cauchy_atomic_exchange_u64(cauchy_atomic_u64_t* ptr, u64 val) {
    return atomic_exchange_explicit(ptr, val, memory_order_acq_rel);
}

CAUCHY_INLINE void* cauchy_atomic_exchange_ptr(cauchy_atomic_ptr_t* ptr, void* val) {
    return atomic_exchange_explicit(ptr, val, memory_order_acq_rel);
}

/* Memory barriers */
CAUCHY_INLINE void cauchy_atomic_fence_acquire(void) {
    atomic_thread_fence(memory_order_acquire);
}

CAUCHY_INLINE void cauchy_atomic_fence_release(void) {
    atomic_thread_fence(memory_order_release);
}

CAUCHY_INLINE void cauchy_atomic_fence_seq_cst(void) {
    atomic_thread_fence(memory_order_seq_cst);
}

/* Double-width CAS for 128-bit atomics (platform-specific) */
#if CAUCHY_HAS_DWCAS
bool cauchy_atomic_cas_u128(cauchy_atomic_u128_t* ptr,
                            cauchy_u128_t* expected,
                            cauchy_u128_t desired);
                            
cauchy_u128_t cauchy_atomic_load_u128(const cauchy_atomic_u128_t* ptr);
void cauchy_atomic_store_u128(cauchy_atomic_u128_t* ptr, cauchy_u128_t val);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CAUCHY_ATOMIC_H */

