/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * Main Header File
 * 
 * Lock-free CRDT implementation with mathematically proven convergence.
 * Include this header for full access to all CAUCHY functionality.
 */

#ifndef CAUCHY_H
#define CAUCHY_H

/* Core infrastructure */
#include "platform.h"
#include "types.h"
#include "atomic.h"
#include "memory.h"
#include "vclock.h"

/* CRDT types - will be added as implemented */
/* #include "crdt/g_counter.h" */
/* #include "crdt/pn_counter.h" */
/* #include "crdt/lww_register.h" */
/* #include "crdt/g_set.h" */
/* #include "crdt/2p_set.h" */
/* #include "crdt/or_set.h" */
/* #include "crdt/lww_map.h" */
/* #include "crdt/rga.h" */

#ifdef __cplusplus
extern "C" {
#endif

/* Library initialization (call once at startup) */
cauchy_result_t cauchy_init(void);

/* Library cleanup (call before exit) */
void cauchy_shutdown(void);

/* Get library version string */
const char* cauchy_version(void);

/* Get library version components */
void cauchy_version_info(int* major, int* minor, int* patch);

/* Context for a local node in the cluster */
typedef struct cauchy_context {
    cauchy_node_id_t    node_id;
    cauchy_vclock_t     local_clock;
    cauchy_pool_t*      mem_pool;
    cauchy_hazard_domain_t* hazard_domain;
    u64                 op_counter;  /* For UID generation */
} cauchy_context_t;

/* Create a new context for a node */
cauchy_context_t* cauchy_context_create(cauchy_node_id_t node_id);

/* Destroy a context */
void cauchy_context_destroy(cauchy_context_t* ctx);

/* Generate a unique identifier */
cauchy_uid_t cauchy_context_gen_uid(cauchy_context_t* ctx);

/* Get the current logical timestamp */
cauchy_timestamp_t cauchy_context_get_timestamp(const cauchy_context_t* ctx);

/* Increment the local clock (for local operations) */
void cauchy_context_tick(cauchy_context_t* ctx);

/* Merge a received clock into the local clock */
void cauchy_context_merge_clock(cauchy_context_t* ctx, 
                                 const cauchy_vclock_t* remote);

#ifdef __cplusplus
}
#endif

#endif /* CAUCHY_H */

