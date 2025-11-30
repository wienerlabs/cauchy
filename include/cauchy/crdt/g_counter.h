/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * G-Counter (Grow-only Counter) CRDT
 * 
 * Each node maintains a local count. Value is sum across all nodes.
 * Merge operation: element-wise maximum.
 * Properties: Monotonically increasing, commutative, associative, idempotent.
 */

#ifndef CAUCHY_CRDT_G_COUNTER_H
#define CAUCHY_CRDT_G_COUNTER_H

#include "../types.h"
#include "../memory.h"
#include "../vclock.h"

#ifdef __cplusplus
extern "C" {
#endif

/* G-Counter structure - one count per node */
typedef struct CAUCHY_CACHE_ALIGNED cauchy_gcounter {
    u64 counts[CAUCHY_MAX_NODES];
    u32 num_nodes;
    u32 _padding;
} cauchy_gcounter_t;

/* Initialize a G-Counter */
void cauchy_gcounter_init(cauchy_gcounter_t* gc, u32 num_nodes);

/* Create a new G-Counter on heap */
cauchy_gcounter_t* cauchy_gcounter_create(u32 num_nodes);

/* Destroy a heap-allocated G-Counter */
void cauchy_gcounter_destroy(cauchy_gcounter_t* gc);

/* Increment the counter for a specific node */
void cauchy_gcounter_increment(cauchy_gcounter_t* gc, cauchy_node_id_t node_id);

/* Increment by a specific amount */
void cauchy_gcounter_add(cauchy_gcounter_t* gc, cauchy_node_id_t node_id, u64 delta);

/* Get the current value (sum of all node counts) */
u64 cauchy_gcounter_value(const cauchy_gcounter_t* gc);

/* Get count for a specific node */
u64 cauchy_gcounter_get(const cauchy_gcounter_t* gc, cauchy_node_id_t node_id);

/* Merge another G-Counter into this one (element-wise maximum) */
void cauchy_gcounter_merge(cauchy_gcounter_t* dst, const cauchy_gcounter_t* src);

/* Check if two G-Counters have the same state */
bool cauchy_gcounter_equals(const cauchy_gcounter_t* a, const cauchy_gcounter_t* b);

/* Compare G-Counters for ordering (useful for testing convergence) */
cauchy_causality_t cauchy_gcounter_compare(const cauchy_gcounter_t* a,
                                            const cauchy_gcounter_t* b);

/* Copy G-Counter state */
void cauchy_gcounter_copy(cauchy_gcounter_t* dst, const cauchy_gcounter_t* src);

/* Clone a G-Counter */
cauchy_gcounter_t* cauchy_gcounter_clone(const cauchy_gcounter_t* gc);

/* Serialization */
usize cauchy_gcounter_serialized_size(const cauchy_gcounter_t* gc);
usize cauchy_gcounter_serialize(const cauchy_gcounter_t* gc, u8* buffer, usize size);
cauchy_result_t cauchy_gcounter_deserialize(cauchy_gcounter_t* gc, 
                                             const u8* buffer, usize size);

/* Debug output */
void cauchy_gcounter_debug_print(const cauchy_gcounter_t* gc, const char* label);

#ifdef __cplusplus
}
#endif

#endif /* CAUCHY_CRDT_G_COUNTER_H */

