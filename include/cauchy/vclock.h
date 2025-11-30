/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * Vector Clock Implementation
 * 
 * Tracks causal dependencies between operations across distributed nodes.
 * Enables detection of concurrent updates and happens-before relationships.
 */

#ifndef CAUCHY_VCLOCK_H
#define CAUCHY_VCLOCK_H

#include "types.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum nodes in a cluster (can be configured at compile time) */
#ifndef CAUCHY_MAX_NODES
#define CAUCHY_MAX_NODES 64
#endif

/* Vector clock structure - fixed size for efficiency */
typedef struct CAUCHY_CACHE_ALIGNED cauchy_vclock {
    u64 entries[CAUCHY_MAX_NODES];
    u32 num_nodes;    /* Active node count */
    u32 _padding;
} cauchy_vclock_t;

/* Initialize a vector clock (all zeros) */
void cauchy_vclock_init(cauchy_vclock_t* vc, u32 num_nodes);

/* Create a new vector clock on heap */
cauchy_vclock_t* cauchy_vclock_create(u32 num_nodes);

/* Destroy a heap-allocated vector clock */
void cauchy_vclock_destroy(cauchy_vclock_t* vc);

/* Copy a vector clock */
void cauchy_vclock_copy(cauchy_vclock_t* dst, const cauchy_vclock_t* src);

/* Clone a vector clock (allocates new) */
cauchy_vclock_t* cauchy_vclock_clone(const cauchy_vclock_t* vc);

/* Increment the clock for a specific node (local event) */
void cauchy_vclock_increment(cauchy_vclock_t* vc, cauchy_node_id_t node_id);

/* Get the timestamp for a specific node */
u64 cauchy_vclock_get(const cauchy_vclock_t* vc, cauchy_node_id_t node_id);

/* Set the timestamp for a specific node */
void cauchy_vclock_set(cauchy_vclock_t* vc, cauchy_node_id_t node_id, u64 value);

/* Merge two vector clocks (element-wise maximum) */
void cauchy_vclock_merge(cauchy_vclock_t* dst, const cauchy_vclock_t* src);

/* Compare two vector clocks for causality */
cauchy_causality_t cauchy_vclock_compare(const cauchy_vclock_t* a,
                                          const cauchy_vclock_t* b);

/* Check if a happens-before b */
bool cauchy_vclock_happens_before(const cauchy_vclock_t* a,
                                   const cauchy_vclock_t* b);

/* Check if two vector clocks are concurrent */
bool cauchy_vclock_concurrent(const cauchy_vclock_t* a,
                               const cauchy_vclock_t* b);

/* Check if two vector clocks are equal */
bool cauchy_vclock_equals(const cauchy_vclock_t* a,
                           const cauchy_vclock_t* b);

/* Check if vector clock is empty (all zeros) */
bool cauchy_vclock_is_empty(const cauchy_vclock_t* vc);

/* Get sum of all entries (useful for debugging) */
u64 cauchy_vclock_sum(const cauchy_vclock_t* vc);

/* Calculate minimum vector clock across all entries */
void cauchy_vclock_min(cauchy_vclock_t* dst, const cauchy_vclock_t* src);

/* Prune entries below threshold (garbage collection) */
u32 cauchy_vclock_prune(cauchy_vclock_t* vc, const cauchy_vclock_t* min_vc);

/* Serialize vector clock to buffer */
usize cauchy_vclock_serialize(const cauchy_vclock_t* vc, 
                               u8* buffer, 
                               usize buffer_size);

/* Deserialize vector clock from buffer */
cauchy_result_t cauchy_vclock_deserialize(cauchy_vclock_t* vc,
                                           const u8* buffer,
                                           usize buffer_size);

/* Get serialized size */
usize cauchy_vclock_serialized_size(const cauchy_vclock_t* vc);

/* Debug: print vector clock to stderr */
void cauchy_vclock_debug_print(const cauchy_vclock_t* vc, const char* label);

#ifdef __cplusplus
}
#endif

#endif /* CAUCHY_VCLOCK_H */

