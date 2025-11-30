/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * OR-Set (Observed-Remove Set) CRDT
 * 
 * Set supporting add and remove with add-wins semantics.
 * Each add operation creates a unique tag.
 * Remove only affects tags observed at remove time.
 * Concurrent adds are preserved after merge.
 */

#ifndef CAUCHY_CRDT_OR_SET_H
#define CAUCHY_CRDT_OR_SET_H

#include "../types.h"
#include "../memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Tagged element in OR-Set */
typedef struct cauchy_orset_entry {
    u8*               data;
    usize             size;
    u64               hash;
    cauchy_uid_t      tag;      /* Unique identifier for this add operation */
    bool              removed;  /* Tombstone flag */
    struct cauchy_orset_entry* next;
} cauchy_orset_entry_t;

/* OR-Set structure */
typedef struct cauchy_orset {
    cauchy_orset_entry_t** buckets;
    usize                  num_buckets;
    usize                  entry_count;  /* Total entries including tombstones */
    usize                  active_count; /* Active (non-removed) entries */
    cauchy_pool_t*         entry_pool;
    cauchy_node_id_t       node_id;
    cauchy_timestamp_t     timestamp;    /* For generating unique tags */
} cauchy_orset_t;

/* Initialize an OR-Set */
cauchy_result_t cauchy_orset_init(cauchy_orset_t* set, usize initial_capacity,
                                   cauchy_node_id_t node_id);

/* Create a new OR-Set on heap */
cauchy_orset_t* cauchy_orset_create(usize initial_capacity, cauchy_node_id_t node_id);

/* Destroy an OR-Set */
void cauchy_orset_destroy(cauchy_orset_t* set);

/* Add an element (creates new unique tag) */
cauchy_result_t cauchy_orset_add(cauchy_orset_t* set, const void* data, usize size);

/* Remove an element (marks all observed tags as removed) */
cauchy_result_t cauchy_orset_remove(cauchy_orset_t* set, const void* data, usize size);

/* Check if element exists (any active tag) */
bool cauchy_orset_contains(const cauchy_orset_t* set, const void* data, usize size);

/* Get count of unique active elements */
usize cauchy_orset_count(const cauchy_orset_t* set);

/* Check if set is empty */
bool cauchy_orset_is_empty(const cauchy_orset_t* set);

/* Merge another OR-Set (add-wins semantics) */
cauchy_result_t cauchy_orset_merge(cauchy_orset_t* dst, const cauchy_orset_t* src);

/* Check equality */
bool cauchy_orset_equals(const cauchy_orset_t* a, const cauchy_orset_t* b);

/* Iterator for unique active elements */
typedef struct cauchy_orset_iter {
    const cauchy_orset_t*    set;
    usize                    bucket_idx;
    cauchy_orset_entry_t*    current;
    const void*              last_data;  /* To skip duplicate values */
    usize                    last_size;
} cauchy_orset_iter_t;

void cauchy_orset_iter_init(cauchy_orset_iter_t* iter, const cauchy_orset_t* set);
bool cauchy_orset_iter_next(cauchy_orset_iter_t* iter, const void** data, usize* size);

/* Convenience for strings */
cauchy_result_t cauchy_orset_add_string(cauchy_orset_t* set, const char* str);
cauchy_result_t cauchy_orset_remove_string(cauchy_orset_t* set, const char* str);
bool cauchy_orset_contains_string(const cauchy_orset_t* set, const char* str);

/* Garbage collection: remove entries where all tags are tombstoned */
usize cauchy_orset_gc(cauchy_orset_t* set);

/* Debug output */
void cauchy_orset_debug_print(const cauchy_orset_t* set, const char* label);

#ifdef __cplusplus
}
#endif

#endif /* CAUCHY_CRDT_OR_SET_H */

