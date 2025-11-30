/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * Memory Management
 * 
 * Lock-free memory pool allocator with cache-aligned blocks.
 * Implements hazard pointers for safe memory reclamation.
 */

#ifndef CAUCHY_MEMORY_H
#define CAUCHY_MEMORY_H

#include "atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct cauchy_pool cauchy_pool_t;
typedef struct cauchy_hazard_domain cauchy_hazard_domain_t;

/* Pool configuration */
typedef struct cauchy_pool_config {
    usize block_size;      /* Size of each block (will be rounded up to alignment) */
    usize initial_blocks;  /* Number of blocks to pre-allocate */
    usize max_blocks;      /* Maximum blocks (0 = unlimited) */
    usize alignment;       /* Memory alignment (default: cache line) */
} cauchy_pool_config_t;

/* Default configuration */
#define CAUCHY_POOL_CONFIG_DEFAULT { \
    .block_size = 64,                \
    .initial_blocks = 1024,          \
    .max_blocks = 0,                 \
    .alignment = CAUCHY_CACHE_LINE_SIZE \
}

/* Pool statistics */
typedef struct cauchy_pool_stats {
    u64 allocated;    /* Total blocks allocated */
    u64 freed;        /* Total blocks freed */
    u64 in_use;       /* Currently in use */
    u64 peak_use;     /* Peak concurrent usage */
    u64 total_allocs; /* Total allocation calls */
    u64 contention;   /* CAS retries (contention indicator) */
} cauchy_pool_stats_t;

/* Create a memory pool */
cauchy_pool_t* cauchy_pool_create(const cauchy_pool_config_t* config);

/* Destroy a memory pool (all blocks must be freed) */
void cauchy_pool_destroy(cauchy_pool_t* pool);

/* Allocate a block from the pool (lock-free) */
void* cauchy_pool_alloc(cauchy_pool_t* pool);

/* Free a block back to the pool (lock-free) */
void cauchy_pool_free(cauchy_pool_t* pool, void* block);

/* Get pool statistics */
cauchy_pool_stats_t cauchy_pool_get_stats(const cauchy_pool_t* pool);

/* ============================================================
 * Hazard Pointers for Safe Memory Reclamation
 * ============================================================ */

/* Maximum hazard pointers per thread */
#define CAUCHY_MAX_HAZARD_POINTERS 4

/* Maximum threads using hazard domain */
#define CAUCHY_MAX_HAZARD_THREADS 128

/* Retired node callback for custom cleanup */
typedef void (*cauchy_retire_fn)(void* node, void* ctx);

/* Create hazard pointer domain */
cauchy_hazard_domain_t* cauchy_hazard_domain_create(void);

/* Destroy hazard pointer domain */
void cauchy_hazard_domain_destroy(cauchy_hazard_domain_t* domain);

/* Protect a pointer (announce intent to access) */
void* cauchy_hazard_protect(cauchy_hazard_domain_t* domain,
                            int hp_index,
                            cauchy_atomic_ptr_t* pptr);

/* Clear hazard pointer protection */
void cauchy_hazard_clear(cauchy_hazard_domain_t* domain, int hp_index);

/* Retire a node (defer deletion until safe) */
void cauchy_hazard_retire(cauchy_hazard_domain_t* domain,
                          void* node,
                          cauchy_retire_fn retire_fn,
                          void* ctx);

/* Try to reclaim retired nodes */
usize cauchy_hazard_reclaim(cauchy_hazard_domain_t* domain);

/* ============================================================
 * General Memory Utilities  
 * ============================================================ */

/* Allocate cache-aligned memory */
void* cauchy_aligned_alloc(usize size, usize alignment);

/* Free aligned memory */
void cauchy_aligned_free(void* ptr);

/* Zero memory (volatile to prevent optimization) */
void cauchy_secure_zero(void* ptr, usize size);

#ifdef __cplusplus
}
#endif

#endif /* CAUCHY_MEMORY_H */

