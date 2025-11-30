/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * 2P-Set (Two-Phase Set) CRDT
 * 
 * Set with add and remove operations using two G-Sets.
 * Remove operation is permanent (tombstone semantics).
 * Once removed, an element cannot be re-added.
 */

#ifndef CAUCHY_CRDT_2P_SET_H
#define CAUCHY_CRDT_2P_SET_H

#include "g_set.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 2P-Set structure: pair of G-Sets */
typedef struct cauchy_2pset {
    cauchy_gset_t* added;    /* Elements that have been added */
    cauchy_gset_t* removed;  /* Elements that have been removed (tombstones) */
} cauchy_2pset_t;

/* Initialize a 2P-Set */
cauchy_result_t cauchy_2pset_init(cauchy_2pset_t* set, usize initial_capacity);

/* Create a new 2P-Set on heap */
cauchy_2pset_t* cauchy_2pset_create(usize initial_capacity);

/* Destroy a 2P-Set */
void cauchy_2pset_destroy(cauchy_2pset_t* set);

/* Add an element */
cauchy_result_t cauchy_2pset_add(cauchy_2pset_t* set, const void* data, usize size);

/* Remove an element (permanent) */
cauchy_result_t cauchy_2pset_remove(cauchy_2pset_t* set, const void* data, usize size);

/* Check if element is in set (added and not removed) */
bool cauchy_2pset_contains(const cauchy_2pset_t* set, const void* data, usize size);

/* Check if element was ever added */
bool cauchy_2pset_was_added(const cauchy_2pset_t* set, const void* data, usize size);

/* Check if element was removed (tombstoned) */
bool cauchy_2pset_was_removed(const cauchy_2pset_t* set, const void* data, usize size);

/* Get count of active elements */
usize cauchy_2pset_count(const cauchy_2pset_t* set);

/* Check if set is empty */
bool cauchy_2pset_is_empty(const cauchy_2pset_t* set);

/* Merge another 2P-Set */
cauchy_result_t cauchy_2pset_merge(cauchy_2pset_t* dst, const cauchy_2pset_t* src);

/* Check equality */
bool cauchy_2pset_equals(const cauchy_2pset_t* a, const cauchy_2pset_t* b);

/* Convenience for strings */
cauchy_result_t cauchy_2pset_add_string(cauchy_2pset_t* set, const char* str);
cauchy_result_t cauchy_2pset_remove_string(cauchy_2pset_t* set, const char* str);
bool cauchy_2pset_contains_string(const cauchy_2pset_t* set, const char* str);

/* Debug output */
void cauchy_2pset_debug_print(const cauchy_2pset_t* set, const char* label);

#ifdef __cplusplus
}
#endif

#endif /* CAUCHY_CRDT_2P_SET_H */

