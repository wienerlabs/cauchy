/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * Hazard Pointer Implementation for Safe Memory Reclamation
 */

#include "cauchy/memory.h"
#include <stdlib.h>
#include <string.h>

/* Retired node awaiting reclamation */
typedef struct retired_node {
    void*            ptr;
    cauchy_retire_fn retire_fn;
    void*            ctx;
    struct retired_node* next;
} retired_node_t;

/* Per-thread hazard record */
typedef struct hazard_record {
    cauchy_atomic_ptr_t hazards[CAUCHY_MAX_HAZARD_POINTERS];
    cauchy_atomic_bool_t active;
    retired_node_t* retired_list;
    u32 retired_count;
    struct hazard_record* next;
} hazard_record_t;

/* Hazard pointer domain */
struct cauchy_hazard_domain {
    cauchy_atomic_ptr_t head;         /* List of hazard records */
    cauchy_atomic_u32_t record_count;
    cauchy_pool_t*      retired_pool; /* Pool for retired nodes */
};

/* Thread-local hazard record */
static _Thread_local hazard_record_t* tls_hazard_record = NULL;
static _Thread_local cauchy_hazard_domain_t* tls_domain = NULL;

static hazard_record_t* get_hazard_record(cauchy_hazard_domain_t* domain) {
    if (tls_domain == domain && tls_hazard_record) {
        return tls_hazard_record;
    }

    hazard_record_t* rec = cauchy_atomic_load_ptr(&domain->head);
    while (rec) {
        bool expected = false;
        if (atomic_compare_exchange_strong(&rec->active, &expected, true)) {
            tls_hazard_record = rec;
            tls_domain = domain;
            return rec;
        }
        rec = rec->next;
    }

    rec = cauchy_aligned_alloc(sizeof(hazard_record_t), CAUCHY_CACHE_LINE_SIZE);
    if (!rec) return NULL;

    memset(rec, 0, sizeof(hazard_record_t));
    atomic_init(&rec->active, true);

    hazard_record_t* head;
    do {
        head = cauchy_atomic_load_ptr(&domain->head);
        rec->next = head;
    } while (!cauchy_atomic_cas_ptr(&domain->head, (void**)&head, rec));

    cauchy_atomic_fetch_add_u32(&domain->record_count, 1);
    tls_hazard_record = rec;
    tls_domain = domain;
    return rec;
}

cauchy_hazard_domain_t* cauchy_hazard_domain_create(void) {
    cauchy_hazard_domain_t* domain = cauchy_aligned_alloc(
        sizeof(cauchy_hazard_domain_t), CAUCHY_CACHE_LINE_SIZE);
    if (!domain) return NULL;

    memset(domain, 0, sizeof(cauchy_hazard_domain_t));

    cauchy_pool_config_t cfg = {
        .block_size = sizeof(retired_node_t),
        .initial_blocks = 256,
        .max_blocks = 0,
        .alignment = sizeof(void*)
    };
    domain->retired_pool = cauchy_pool_create(&cfg);

    return domain;
}

void cauchy_hazard_domain_destroy(cauchy_hazard_domain_t* domain) {
    if (!domain) return;

    hazard_record_t* rec = cauchy_atomic_load_ptr(&domain->head);
    while (rec) {
        hazard_record_t* next = rec->next;
        retired_node_t* retired = rec->retired_list;
        while (retired) {
            retired_node_t* rn = retired;
            retired = retired->next;
            if (rn->retire_fn) rn->retire_fn(rn->ptr, rn->ctx);
        }
        cauchy_aligned_free(rec);
        rec = next;
    }

    if (domain->retired_pool) cauchy_pool_destroy(domain->retired_pool);
    cauchy_aligned_free(domain);
}

void* cauchy_hazard_protect(cauchy_hazard_domain_t* domain, int hp_index,
                            cauchy_atomic_ptr_t* pptr) {
    if (!domain || hp_index < 0 || hp_index >= CAUCHY_MAX_HAZARD_POINTERS) return NULL;

    hazard_record_t* rec = get_hazard_record(domain);
    if (!rec) return NULL;

    void* ptr;
    do {
        ptr = cauchy_atomic_load_ptr(pptr);
        cauchy_atomic_store_ptr(&rec->hazards[hp_index], ptr);
        cauchy_atomic_fence_seq_cst();
    } while (ptr != cauchy_atomic_load_ptr(pptr));

    return ptr;
}

void cauchy_hazard_clear(cauchy_hazard_domain_t* domain, int hp_index) {
    if (!domain || hp_index < 0 || hp_index >= CAUCHY_MAX_HAZARD_POINTERS) return;
    hazard_record_t* rec = get_hazard_record(domain);
    if (rec) cauchy_atomic_store_ptr(&rec->hazards[hp_index], NULL);
}

static bool is_hazardous(cauchy_hazard_domain_t* domain, void* ptr) {
    hazard_record_t* rec = cauchy_atomic_load_ptr(&domain->head);
    while (rec) {
        if (atomic_load(&rec->active)) {
            for (int i = 0; i < CAUCHY_MAX_HAZARD_POINTERS; i++) {
                if (cauchy_atomic_load_ptr(&rec->hazards[i]) == ptr) {
                    return true;
                }
            }
        }
        rec = rec->next;
    }
    return false;
}

void cauchy_hazard_retire(cauchy_hazard_domain_t* domain, void* node,
                          cauchy_retire_fn retire_fn, void* ctx) {
    if (!domain || !node) return;

    hazard_record_t* rec = get_hazard_record(domain);
    if (!rec) {
        if (retire_fn) retire_fn(node, ctx);
        return;
    }

    retired_node_t* rn = cauchy_pool_alloc(domain->retired_pool);
    if (!rn) {
        if (retire_fn) retire_fn(node, ctx);
        return;
    }

    rn->ptr = node;
    rn->retire_fn = retire_fn;
    rn->ctx = ctx;
    rn->next = rec->retired_list;
    rec->retired_list = rn;
    rec->retired_count++;

    if (rec->retired_count >= CAUCHY_MAX_HAZARD_THREADS * CAUCHY_MAX_HAZARD_POINTERS * 2) {
        cauchy_hazard_reclaim(domain);
    }
}

usize cauchy_hazard_reclaim(cauchy_hazard_domain_t* domain) {
    if (!domain) return 0;

    hazard_record_t* rec = get_hazard_record(domain);
    if (!rec) return 0;

    usize reclaimed = 0;
    retired_node_t* prev = NULL;
    retired_node_t* curr = rec->retired_list;

    while (curr) {
        retired_node_t* next = curr->next;

        if (!is_hazardous(domain, curr->ptr)) {
            if (curr->retire_fn) {
                curr->retire_fn(curr->ptr, curr->ctx);
            }

            if (prev) prev->next = next;
            else rec->retired_list = next;

            cauchy_pool_free(domain->retired_pool, curr);
            rec->retired_count--;
            reclaimed++;
        } else {
            prev = curr;
        }
        curr = next;
    }

    return reclaimed;
}
