/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * G-Set Implementation
 */

#include "cauchy/crdt/g_set.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static u64 hash_data(const void* data, usize size) {
    const u8* bytes = (const u8*)data;
    u64 hash = 14695981039346656037ULL;  /* FNV-1a offset basis */
    for (usize i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;  /* FNV prime */
    }
    return hash;
}

cauchy_result_t cauchy_gset_init(cauchy_gset_t* set, usize initial_capacity) {
    if (!set) return CAUCHY_ERR_INVALID;
    if (initial_capacity == 0) initial_capacity = 16;

    set->buckets = calloc(initial_capacity, sizeof(cauchy_gset_elem_t*));
    if (!set->buckets) return CAUCHY_ERR_NOMEM;

    set->num_buckets = initial_capacity;
    set->count = 0;

    cauchy_pool_config_t cfg = {
        .block_size = sizeof(cauchy_gset_elem_t) + 64,
        .initial_blocks = 128,
        .max_blocks = 0,
        .alignment = sizeof(void*)
    };
    set->elem_pool = cauchy_pool_create(&cfg);
    if (!set->elem_pool) {
        free(set->buckets);
        return CAUCHY_ERR_NOMEM;
    }
    return CAUCHY_OK;
}

cauchy_gset_t* cauchy_gset_create(usize initial_capacity) {
    cauchy_gset_t* set = malloc(sizeof(cauchy_gset_t));
    if (!set) return NULL;
    if (cauchy_gset_init(set, initial_capacity) != CAUCHY_OK) {
        free(set);
        return NULL;
    }
    return set;
}

void cauchy_gset_destroy(cauchy_gset_t* set) {
    if (!set) return;
    for (usize i = 0; i < set->num_buckets; i++) {
        cauchy_gset_elem_t* elem = set->buckets[i];
        while (elem) {
            cauchy_gset_elem_t* next = elem->next;
            if (elem->data) free(elem->data);
            elem = next;
        }
    }
    free(set->buckets);
    if (set->elem_pool) cauchy_pool_destroy(set->elem_pool);
    free(set);
}

cauchy_result_t cauchy_gset_add(cauchy_gset_t* set, const void* data, usize size) {
    if (!set || !data || size == 0) return CAUCHY_ERR_INVALID;

    u64 h = hash_data(data, size);
    usize idx = h % set->num_buckets;

    cauchy_gset_elem_t* elem = set->buckets[idx];
    while (elem) {
        if (elem->hash == h && elem->size == size &&
            memcmp(elem->data, data, size) == 0) {
            return CAUCHY_OK;  /* Already exists */
        }
        elem = elem->next;
    }

    cauchy_gset_elem_t* new_elem = cauchy_pool_alloc(set->elem_pool);
    if (!new_elem) new_elem = malloc(sizeof(cauchy_gset_elem_t));
    if (!new_elem) return CAUCHY_ERR_NOMEM;

    new_elem->data = malloc(size);
    if (!new_elem->data) {
        cauchy_pool_free(set->elem_pool, new_elem);
        return CAUCHY_ERR_NOMEM;
    }

    memcpy(new_elem->data, data, size);
    new_elem->size = size;
    new_elem->hash = h;
    new_elem->next = set->buckets[idx];
    set->buckets[idx] = new_elem;
    set->count++;
    return CAUCHY_OK;
}

bool cauchy_gset_contains(const cauchy_gset_t* set, const void* data, usize size) {
    if (!set || !data || size == 0) return false;

    u64 h = hash_data(data, size);
    usize idx = h % set->num_buckets;

    cauchy_gset_elem_t* elem = set->buckets[idx];
    while (elem) {
        if (elem->hash == h && elem->size == size &&
            memcmp(elem->data, data, size) == 0) {
            return true;
        }
        elem = elem->next;
    }
    return false;
}

usize cauchy_gset_count(const cauchy_gset_t* set) {
    return set ? set->count : 0;
}

bool cauchy_gset_is_empty(const cauchy_gset_t* set) {
    return !set || set->count == 0;
}

cauchy_result_t cauchy_gset_merge(cauchy_gset_t* dst, const cauchy_gset_t* src) {
    if (!dst || !src) return CAUCHY_ERR_INVALID;

    for (usize i = 0; i < src->num_buckets; i++) {
        cauchy_gset_elem_t* elem = src->buckets[i];
        while (elem) {
            cauchy_result_t res = cauchy_gset_add(dst, elem->data, elem->size);
            if (res != CAUCHY_OK) return res;
            elem = elem->next;
        }
    }
    return CAUCHY_OK;
}

bool cauchy_gset_equals(const cauchy_gset_t* a, const cauchy_gset_t* b) {
    if (!a || !b) return a == b;
    if (a->count != b->count) return false;
    return cauchy_gset_subset(a, b) && cauchy_gset_subset(b, a);
}

bool cauchy_gset_subset(const cauchy_gset_t* a, const cauchy_gset_t* b) {
    if (!a || !b) return false;
    for (usize i = 0; i < a->num_buckets; i++) {
        cauchy_gset_elem_t* elem = a->buckets[i];
        while (elem) {
            if (!cauchy_gset_contains(b, elem->data, elem->size)) return false;
            elem = elem->next;
        }
    }
    return true;
}

void cauchy_gset_iter_init(cauchy_gset_iter_t* iter, const cauchy_gset_t* set) {
    if (!iter) return;
    iter->set = set;
    iter->bucket_idx = 0;
    iter->current = set ? set->buckets[0] : NULL;
    while (set && !iter->current && iter->bucket_idx < set->num_buckets - 1) {
        iter->bucket_idx++;
        iter->current = set->buckets[iter->bucket_idx];
    }
}

bool cauchy_gset_iter_next(cauchy_gset_iter_t* iter, const void** data, usize* size) {
    if (!iter || !iter->set || !iter->current) return false;

    if (data) *data = iter->current->data;
    if (size) *size = iter->current->size;

    iter->current = iter->current->next;
    while (!iter->current && iter->bucket_idx < iter->set->num_buckets - 1) {
        iter->bucket_idx++;
        iter->current = iter->set->buckets[iter->bucket_idx];
    }
    return true;
}

cauchy_result_t cauchy_gset_add_string(cauchy_gset_t* set, const char* str) {
    if (!str) return CAUCHY_ERR_INVALID;
    return cauchy_gset_add(set, str, strlen(str) + 1);
}

bool cauchy_gset_contains_string(const cauchy_gset_t* set, const char* str) {
    if (!str) return false;
    return cauchy_gset_contains(set, str, strlen(str) + 1);
}

void cauchy_gset_debug_print(const cauchy_gset_t* set, const char* label) {
    if (!set) { fprintf(stderr, "%s: (null)\n", label ? label : "gset"); return; }
    fprintf(stderr, "%s: count=%zu\n", label ? label : "gset", set->count);
}

