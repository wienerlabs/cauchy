/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * Memory Management Implementation
 */

#include "cauchy/memory.h"
#include <stdlib.h>
#include <string.h>

/* Lock-free stack node for free list */
typedef struct pool_node {
    struct pool_node* next;
} pool_node_t;

/* Memory pool structure */
struct cauchy_pool {
    cauchy_atomic_ptr_t  free_list;    /* Lock-free stack of free blocks */
    cauchy_atomic_u64_t  allocated;
    cauchy_atomic_u64_t  freed;
    cauchy_atomic_u64_t  peak_use;
    cauchy_atomic_u64_t  total_allocs;
    cauchy_atomic_u64_t  contention;
    usize                block_size;
    usize                alignment;
    usize                max_blocks;
    void*                base_memory;  /* For bulk deallocation */
    usize                base_size;
};

void* cauchy_aligned_alloc(usize size, usize alignment) {
#if defined(CAUCHY_OS_WINDOWS)
    return _aligned_malloc(size, alignment);
#elif defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L
    void* ptr = NULL;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return NULL;
    }
    return ptr;
#else
    void* ptr = malloc(size + alignment + sizeof(void*));
    if (!ptr) return NULL;
    void* aligned = (void*)(((uintptr_t)ptr + alignment + sizeof(void*)) & ~(alignment - 1));
    ((void**)aligned)[-1] = ptr;
    return aligned;
#endif
}

void cauchy_aligned_free(void* ptr) {
    if (!ptr) return;
#if defined(CAUCHY_OS_WINDOWS)
    _aligned_free(ptr);
#elif defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L
    free(ptr);
#else
    free(((void**)ptr)[-1]);
#endif
}

void cauchy_secure_zero(void* ptr, usize size) {
    volatile unsigned char* p = (volatile unsigned char*)ptr;
    while (size--) {
        *p++ = 0;
    }
}

cauchy_pool_t* cauchy_pool_create(const cauchy_pool_config_t* config) {
    cauchy_pool_config_t cfg = config ? *config : (cauchy_pool_config_t)CAUCHY_POOL_CONFIG_DEFAULT;
    
    if (cfg.alignment == 0) cfg.alignment = CAUCHY_CACHE_LINE_SIZE;
    usize actual_block_size = (cfg.block_size + cfg.alignment - 1) & ~(cfg.alignment - 1);
    if (actual_block_size < sizeof(pool_node_t)) {
        actual_block_size = sizeof(pool_node_t);
    }
    
    cauchy_pool_t* pool = cauchy_aligned_alloc(sizeof(cauchy_pool_t), CAUCHY_CACHE_LINE_SIZE);
    if (!pool) return NULL;
    
    memset(pool, 0, sizeof(cauchy_pool_t));
    pool->block_size = actual_block_size;
    pool->alignment = cfg.alignment;
    pool->max_blocks = cfg.max_blocks;
    atomic_init(&pool->free_list, NULL);
    
    if (cfg.initial_blocks > 0) {
        usize total_size = actual_block_size * cfg.initial_blocks;
        pool->base_memory = cauchy_aligned_alloc(total_size, cfg.alignment);
        if (!pool->base_memory) {
            cauchy_aligned_free(pool);
            return NULL;
        }
        pool->base_size = total_size;
        
        u8* block = (u8*)pool->base_memory;
        for (usize i = 0; i < cfg.initial_blocks; i++) {
            pool_node_t* node = (pool_node_t*)block;
            node->next = cauchy_atomic_load_ptr(&pool->free_list);
            cauchy_atomic_store_ptr(&pool->free_list, node);
            block += actual_block_size;
        }
        atomic_store(&pool->allocated, cfg.initial_blocks);
    }
    
    return pool;
}

void cauchy_pool_destroy(cauchy_pool_t* pool) {
    if (!pool) return;
    if (pool->base_memory) {
        cauchy_aligned_free(pool->base_memory);
    }
    cauchy_aligned_free(pool);
}

void* cauchy_pool_alloc(cauchy_pool_t* pool) {
    if (!pool) return NULL;
    
    cauchy_atomic_fetch_add_u64(&pool->total_allocs, 1);
    
    pool_node_t* node;
    pool_node_t* next;
    do {
        node = cauchy_atomic_load_ptr(&pool->free_list);
        if (!node) {
            void* new_block = cauchy_aligned_alloc(pool->block_size, pool->alignment);
            if (new_block) {
                cauchy_atomic_fetch_add_u64(&pool->allocated, 1);
            }
            return new_block;
        }
        next = node->next;
    } while (!cauchy_atomic_cas_ptr(&pool->free_list, (void**)&node, next));
    
    u64 in_use = cauchy_atomic_load_u64(&pool->allocated) - cauchy_atomic_load_u64(&pool->freed);
    u64 peak = cauchy_atomic_load_u64(&pool->peak_use);
    while (in_use > peak) {
        if (cauchy_atomic_cas_u64(&pool->peak_use, &peak, in_use)) break;
    }
    
    return node;
}

void cauchy_pool_free(cauchy_pool_t* pool, void* block) {
    if (!pool || !block) return;
    
    pool_node_t* node = (pool_node_t*)block;
    pool_node_t* head;
    do {
        head = cauchy_atomic_load_ptr(&pool->free_list);
        node->next = head;
    } while (!cauchy_atomic_cas_ptr(&pool->free_list, (void**)&head, node));
    
    cauchy_atomic_fetch_add_u64(&pool->freed, 1);
}

cauchy_pool_stats_t cauchy_pool_get_stats(const cauchy_pool_t* pool) {
    cauchy_pool_stats_t stats = {0};
    if (!pool) return stats;
    
    stats.allocated = cauchy_atomic_load_u64(&pool->allocated);
    stats.freed = cauchy_atomic_load_u64(&pool->freed);
    stats.in_use = stats.allocated - stats.freed;
    stats.peak_use = cauchy_atomic_load_u64(&pool->peak_use);
    stats.total_allocs = cauchy_atomic_load_u64(&pool->total_allocs);
    stats.contention = cauchy_atomic_load_u64(&pool->contention);
    return stats;
}

