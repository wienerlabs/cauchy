/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * G-Set (Grow-only Set) CRDT
 * 
 * Set that only supports add operations (no remove).
 * Merge operation: set union.
 * Elements are never removed once added.
 */

#ifndef CAUCHY_CRDT_G_SET_H
#define CAUCHY_CRDT_G_SET_H

#include "../types.h"
#include "../memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Element in the set */
typedef struct cauchy_gset_elem {
    u8*    data;
    usize  size;
    u64    hash;  /* For fast comparison */
    struct cauchy_gset_elem* next;
} cauchy_gset_elem_t;

/* G-Set structure */
typedef struct cauchy_gset {
    cauchy_gset_elem_t** buckets;  /* Hash table buckets */
    usize                num_buckets;
    usize                count;
    cauchy_pool_t*       elem_pool;
} cauchy_gset_t;

/* Initialize a G-Set */
cauchy_result_t cauchy_gset_init(cauchy_gset_t* set, usize initial_capacity);

/* Create a new G-Set on heap */
cauchy_gset_t* cauchy_gset_create(usize initial_capacity);

/* Destroy a G-Set */
void cauchy_gset_destroy(cauchy_gset_t* set);

/* Add an element to the set */
cauchy_result_t cauchy_gset_add(cauchy_gset_t* set, const void* data, usize size);

/* Check if element exists */
bool cauchy_gset_contains(const cauchy_gset_t* set, const void* data, usize size);

/* Get element count */
usize cauchy_gset_count(const cauchy_gset_t* set);

/* Check if set is empty */
bool cauchy_gset_is_empty(const cauchy_gset_t* set);

/* Merge another set (union) */
cauchy_result_t cauchy_gset_merge(cauchy_gset_t* dst, const cauchy_gset_t* src);

/* Check equality */
bool cauchy_gset_equals(const cauchy_gset_t* a, const cauchy_gset_t* b);

/* Check if a is subset of b */
bool cauchy_gset_subset(const cauchy_gset_t* a, const cauchy_gset_t* b);

/* Iterator */
typedef struct cauchy_gset_iter {
    const cauchy_gset_t* set;
    usize                bucket_idx;
    cauchy_gset_elem_t*  current;
} cauchy_gset_iter_t;

void cauchy_gset_iter_init(cauchy_gset_iter_t* iter, const cauchy_gset_t* set);
bool cauchy_gset_iter_next(cauchy_gset_iter_t* iter, const void** data, usize* size);

/* Serialization */
usize cauchy_gset_serialized_size(const cauchy_gset_t* set);
usize cauchy_gset_serialize(const cauchy_gset_t* set, u8* buffer, usize size);
cauchy_result_t cauchy_gset_deserialize(cauchy_gset_t* set, const u8* buffer, usize size);

/* Convenience for string sets */
cauchy_result_t cauchy_gset_add_string(cauchy_gset_t* set, const char* str);
bool cauchy_gset_contains_string(const cauchy_gset_t* set, const char* str);

/* Debug output */
void cauchy_gset_debug_print(const cauchy_gset_t* set, const char* label);

#ifdef __cplusplus
}
#endif

#endif /* CAUCHY_CRDT_G_SET_H */

