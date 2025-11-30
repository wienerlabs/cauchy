/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * Main Library Implementation
 */

#include "cauchy/cauchy.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char* CAUCHY_VERSION_STR = "0.1.0";
static cauchy_atomic_bool_t cauchy_initialized = false;

cauchy_result_t cauchy_init(void) {
    bool expected = false;
    if (!atomic_compare_exchange_strong(&cauchy_initialized, &expected, true)) {
        return CAUCHY_OK;
    }
    return CAUCHY_OK;
}

void cauchy_shutdown(void) {
    atomic_store(&cauchy_initialized, false);
}

const char* cauchy_version(void) {
    return CAUCHY_VERSION_STR;
}

void cauchy_version_info(int* major, int* minor, int* patch) {
    if (major) *major = CAUCHY_VERSION_MAJOR;
    if (minor) *minor = CAUCHY_VERSION_MINOR;
    if (patch) *patch = CAUCHY_VERSION_PATCH;
}

cauchy_context_t* cauchy_context_create(cauchy_node_id_t node_id) {
    cauchy_context_t* ctx = cauchy_aligned_alloc(
        sizeof(cauchy_context_t), CAUCHY_CACHE_LINE_SIZE);
    if (!ctx) return NULL;

    memset(ctx, 0, sizeof(cauchy_context_t));
    ctx->node_id = node_id;
    cauchy_vclock_init(&ctx->local_clock, CAUCHY_MAX_NODES);

    cauchy_pool_config_t pool_cfg = CAUCHY_POOL_CONFIG_DEFAULT;
    pool_cfg.block_size = 128;
    pool_cfg.initial_blocks = 4096;
    ctx->mem_pool = cauchy_pool_create(&pool_cfg);
    if (!ctx->mem_pool) {
        cauchy_aligned_free(ctx);
        return NULL;
    }

    ctx->hazard_domain = cauchy_hazard_domain_create();
    if (!ctx->hazard_domain) {
        cauchy_pool_destroy(ctx->mem_pool);
        cauchy_aligned_free(ctx);
        return NULL;
    }

    ctx->op_counter = 0;
    return ctx;
}

void cauchy_context_destroy(cauchy_context_t* ctx) {
    if (!ctx) return;

    if (ctx->hazard_domain) {
        cauchy_hazard_domain_destroy(ctx->hazard_domain);
    }
    if (ctx->mem_pool) {
        cauchy_pool_destroy(ctx->mem_pool);
    }
    cauchy_aligned_free(ctx);
}

cauchy_uid_t cauchy_context_gen_uid(cauchy_context_t* ctx) {
    if (!ctx) {
        return (cauchy_uid_t){ .node_id = 0, .timestamp = 0 };
    }
    
    cauchy_vclock_increment(&ctx->local_clock, ctx->node_id);
    ctx->op_counter++;
    
    return cauchy_uid_create(
        ctx->node_id,
        cauchy_vclock_get(&ctx->local_clock, ctx->node_id)
    );
}

cauchy_timestamp_t cauchy_context_get_timestamp(const cauchy_context_t* ctx) {
    if (!ctx) return 0;
    return cauchy_vclock_get(&ctx->local_clock, ctx->node_id);
}

void cauchy_context_tick(cauchy_context_t* ctx) {
    if (!ctx) return;
    cauchy_vclock_increment(&ctx->local_clock, ctx->node_id);
}

void cauchy_context_merge_clock(cauchy_context_t* ctx, const cauchy_vclock_t* remote) {
    if (!ctx || !remote) return;
    cauchy_vclock_merge(&ctx->local_clock, remote);
    cauchy_vclock_increment(&ctx->local_clock, ctx->node_id);
}

