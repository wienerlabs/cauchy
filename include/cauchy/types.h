/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * Core Type Definitions
 * 
 * Provides fixed-width integer types, result codes, and fundamental
 * structures used throughout the CRDT implementation.
 */

#ifndef CAUCHY_TYPES_H
#define CAUCHY_TYPES_H

#include "platform.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Fixed-width types for explicit sizing */
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

/* Pointer-sized integers */
typedef uintptr_t usize;
typedef intptr_t  isize;

/* Node identifier (unique across cluster) */
typedef u64 cauchy_node_id_t;

/* Logical timestamp for ordering */
typedef u64 cauchy_timestamp_t;

/* Unique identifier for elements (used in OR-Set, RGA) */
typedef struct cauchy_uid {
    cauchy_node_id_t   node_id;
    cauchy_timestamp_t timestamp;
} cauchy_uid_t;

/* Result codes for operations */
typedef enum cauchy_result {
    CAUCHY_OK              =  0,
    CAUCHY_ERR_NOMEM       = -1,
    CAUCHY_ERR_INVALID     = -2,
    CAUCHY_ERR_NOTFOUND    = -3,
    CAUCHY_ERR_EXISTS      = -4,
    CAUCHY_ERR_FULL        = -5,
    CAUCHY_ERR_EMPTY       = -6,
    CAUCHY_ERR_TIMEOUT     = -7,
    CAUCHY_ERR_CONCURRENT  = -8,  /* CAS failure due to concurrent modification */
    CAUCHY_ERR_CAUSAL      = -9,  /* Causal dependency not satisfied */
    CAUCHY_ERR_NETWORK     = -10,
    CAUCHY_ERR_INTERNAL    = -11
} cauchy_result_t;

/* Causality relationship between two events */
typedef enum cauchy_causality {
    CAUCHY_HAPPENS_BEFORE = -1,
    CAUCHY_CONCURRENT     =  0,
    CAUCHY_HAPPENS_AFTER  =  1,
    CAUCHY_EQUAL          =  2
} cauchy_causality_t;

/* CRDT type enumeration */
typedef enum cauchy_crdt_type {
    CAUCHY_CRDT_G_COUNTER,
    CAUCHY_CRDT_PN_COUNTER,
    CAUCHY_CRDT_LWW_REGISTER,
    CAUCHY_CRDT_G_SET,
    CAUCHY_CRDT_2P_SET,
    CAUCHY_CRDT_OR_SET,
    CAUCHY_CRDT_LWW_MAP,
    CAUCHY_CRDT_RGA,
    CAUCHY_CRDT_TYPE_COUNT
} cauchy_crdt_type_t;

/* 128-bit value for double-width CAS operations */
typedef struct CAUCHY_ALIGNED(16) cauchy_u128 {
    u64 lo;
    u64 hi;
} cauchy_u128_t;

/* Tagged pointer for ABA prevention */
typedef struct CAUCHY_ALIGNED(16) cauchy_tagged_ptr {
    void* ptr;
    u64   tag;
} cauchy_tagged_ptr_t;

/* Atomic tagged pointer */
typedef _Atomic cauchy_u128_t cauchy_atomic_u128_t;

/* Helper to create UID */
CAUCHY_INLINE cauchy_uid_t cauchy_uid_create(cauchy_node_id_t node, 
                                              cauchy_timestamp_t ts) {
    return (cauchy_uid_t){ .node_id = node, .timestamp = ts };
}

/* Compare two UIDs */
CAUCHY_INLINE int cauchy_uid_compare(const cauchy_uid_t* a, 
                                      const cauchy_uid_t* b) {
    if (a->timestamp != b->timestamp) {
        return (a->timestamp < b->timestamp) ? -1 : 1;
    }
    if (a->node_id != b->node_id) {
        return (a->node_id < b->node_id) ? -1 : 1;
    }
    return 0;
}

/* Check UID equality */
CAUCHY_INLINE bool cauchy_uid_equals(const cauchy_uid_t* a, 
                                      const cauchy_uid_t* b) {
    return a->node_id == b->node_id && a->timestamp == b->timestamp;
}

/* Result code to string */
const char* cauchy_result_str(cauchy_result_t result);

/* CRDT type to string */
const char* cauchy_crdt_type_str(cauchy_crdt_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* CAUCHY_TYPES_H */

