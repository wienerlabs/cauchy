/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * PN-Counter (Positive-Negative Counter) CRDT
 * 
 * Supports both increment and decrement operations.
 * Implemented as pair of G-Counters: one for increments, one for decrements.
 * Value = sum(increments) - sum(decrements)
 */

#ifndef CAUCHY_CRDT_PN_COUNTER_H
#define CAUCHY_CRDT_PN_COUNTER_H

#include "g_counter.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PN-Counter structure - pair of G-Counters */
typedef struct CAUCHY_CACHE_ALIGNED cauchy_pncounter {
    cauchy_gcounter_t positive;  /* Increments */
    cauchy_gcounter_t negative;  /* Decrements */
} cauchy_pncounter_t;

/* Initialize a PN-Counter */
void cauchy_pncounter_init(cauchy_pncounter_t* pn, u32 num_nodes);

/* Create a new PN-Counter on heap */
cauchy_pncounter_t* cauchy_pncounter_create(u32 num_nodes);

/* Destroy a heap-allocated PN-Counter */
void cauchy_pncounter_destroy(cauchy_pncounter_t* pn);

/* Increment the counter */
void cauchy_pncounter_increment(cauchy_pncounter_t* pn, cauchy_node_id_t node_id);

/* Decrement the counter */
void cauchy_pncounter_decrement(cauchy_pncounter_t* pn, cauchy_node_id_t node_id);

/* Add a positive delta */
void cauchy_pncounter_add(cauchy_pncounter_t* pn, cauchy_node_id_t node_id, i64 delta);

/* Get the current value (can be negative) */
i64 cauchy_pncounter_value(const cauchy_pncounter_t* pn);

/* Get positive sum */
u64 cauchy_pncounter_positive(const cauchy_pncounter_t* pn);

/* Get negative sum */
u64 cauchy_pncounter_negative(const cauchy_pncounter_t* pn);

/* Merge another PN-Counter */
void cauchy_pncounter_merge(cauchy_pncounter_t* dst, const cauchy_pncounter_t* src);

/* Check equality */
bool cauchy_pncounter_equals(const cauchy_pncounter_t* a, const cauchy_pncounter_t* b);

/* Copy state */
void cauchy_pncounter_copy(cauchy_pncounter_t* dst, const cauchy_pncounter_t* src);

/* Clone */
cauchy_pncounter_t* cauchy_pncounter_clone(const cauchy_pncounter_t* pn);

/* Serialization */
usize cauchy_pncounter_serialized_size(const cauchy_pncounter_t* pn);
usize cauchy_pncounter_serialize(const cauchy_pncounter_t* pn, u8* buffer, usize size);
cauchy_result_t cauchy_pncounter_deserialize(cauchy_pncounter_t* pn,
                                              const u8* buffer, usize size);

/* Debug output */
void cauchy_pncounter_debug_print(const cauchy_pncounter_t* pn, const char* label);

#ifdef __cplusplus
}
#endif

#endif /* CAUCHY_CRDT_PN_COUNTER_H */

