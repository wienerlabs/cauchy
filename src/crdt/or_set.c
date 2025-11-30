/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * OR-Set Implementation
 */

#include "cauchy/crdt/or_set.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static u64 hash_data(const void* data, usize size) {
    const u8* bytes = (const u8*)data;
    u64 hash = 14695981039346656037ULL;
    for (usize i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

cauchy_result_t cauchy_orset_init(cauchy_orset_t* set, usize initial_capacity,
                                   cauchy_node_id_t node_id) {
    if (!set) return CAUCHY_ERR_INVALID;
    if (initial_capacity == 0) initial_capacity = 16;

    set->buckets = calloc(initial_capacity, sizeof(cauchy_orset_entry_t*));
    if (!set->buckets) return CAUCHY_ERR_NOMEM;

    set->num_buckets = initial_capacity;
    set->entry_count = 0;
    set->active_count = 0;
    set->node_id = node_id;
    set->timestamp = 0;

    cauchy_pool_config_t cfg = {
        .block_size = sizeof(cauchy_orset_entry_t) + 64,
        .initial_blocks = 128,
        .max_blocks = 0,
        .alignment = sizeof(void*)
    };
    set->entry_pool = cauchy_pool_create(&cfg);
    if (!set->entry_pool) {
        free(set->buckets);
        return CAUCHY_ERR_NOMEM;
    }
    return CAUCHY_OK;
}

cauchy_orset_t* cauchy_orset_create(usize initial_capacity, cauchy_node_id_t node_id) {
    cauchy_orset_t* set = malloc(sizeof(cauchy_orset_t));
    if (!set) return NULL;
    if (cauchy_orset_init(set, initial_capacity, node_id) != CAUCHY_OK) {
        free(set);
        return NULL;
    }
    return set;
}

void cauchy_orset_destroy(cauchy_orset_t* set) {
    if (!set) return;
    for (usize i = 0; i < set->num_buckets; i++) {
        cauchy_orset_entry_t* entry = set->buckets[i];
        while (entry) {
            cauchy_orset_entry_t* next = entry->next;
            if (entry->data) free(entry->data);
            entry = next;
        }
    }
    free(set->buckets);
    if (set->entry_pool) cauchy_pool_destroy(set->entry_pool);
    free(set);
}

cauchy_result_t cauchy_orset_add(cauchy_orset_t* set, const void* data, usize size) {
    if (!set || !data || size == 0) return CAUCHY_ERR_INVALID;

    u64 h = hash_data(data, size);
    usize idx = h % set->num_buckets;

    cauchy_orset_entry_t* entry = cauchy_pool_alloc(set->entry_pool);
    if (!entry) entry = malloc(sizeof(cauchy_orset_entry_t));
    if (!entry) return CAUCHY_ERR_NOMEM;

    entry->data = malloc(size);
    if (!entry->data) {
        cauchy_pool_free(set->entry_pool, entry);
        return CAUCHY_ERR_NOMEM;
    }

    memcpy(entry->data, data, size);
    entry->size = size;
    entry->hash = h;
    entry->tag = cauchy_uid_create(set->node_id, ++set->timestamp);
    entry->removed = false;
    entry->next = set->buckets[idx];
    set->buckets[idx] = entry;
    set->entry_count++;
    set->active_count++;
    return CAUCHY_OK;
}

cauchy_result_t cauchy_orset_remove(cauchy_orset_t* set, const void* data, usize size) {
    if (!set || !data || size == 0) return CAUCHY_ERR_INVALID;

    u64 h = hash_data(data, size);
    usize idx = h % set->num_buckets;
    bool found = false;

    cauchy_orset_entry_t* entry = set->buckets[idx];
    while (entry) {
        if (!entry->removed && entry->hash == h && entry->size == size &&
            memcmp(entry->data, data, size) == 0) {
            entry->removed = true;
            set->active_count--;
            found = true;
        }
        entry = entry->next;
    }
    return found ? CAUCHY_OK : CAUCHY_ERR_NOTFOUND;
}

bool cauchy_orset_contains(const cauchy_orset_t* set, const void* data, usize size) {
    if (!set || !data || size == 0) return false;

    u64 h = hash_data(data, size);
    usize idx = h % set->num_buckets;

    cauchy_orset_entry_t* entry = set->buckets[idx];
    while (entry) {
        if (!entry->removed && entry->hash == h && entry->size == size &&
            memcmp(entry->data, data, size) == 0) {
            return true;
        }
        entry = entry->next;
    }
    return false;
}

usize cauchy_orset_count(const cauchy_orset_t* set) {
    return set ? set->active_count : 0;
}

bool cauchy_orset_is_empty(const cauchy_orset_t* set) {
    return !set || set->active_count == 0;
}

static cauchy_orset_entry_t* find_entry_by_tag(cauchy_orset_t* set,
                                                u64 hash,
                                                const cauchy_uid_t* tag) {
    usize idx = hash % set->num_buckets;
    cauchy_orset_entry_t* entry = set->buckets[idx];
    while (entry) {
        if (entry->hash == hash && cauchy_uid_equals(&entry->tag, tag)) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

cauchy_result_t cauchy_orset_merge(cauchy_orset_t* dst, const cauchy_orset_t* src) {
    if (!dst || !src) return CAUCHY_ERR_INVALID;

    for (usize i = 0; i < src->num_buckets; i++) {
        cauchy_orset_entry_t* src_entry = src->buckets[i];
        while (src_entry) {
            cauchy_orset_entry_t* existing = find_entry_by_tag(dst, src_entry->hash, &src_entry->tag);

            if (existing) {
                if (src_entry->removed && !existing->removed) {
                    existing->removed = true;
                    dst->active_count--;
                }
            } else {
                usize idx = src_entry->hash % dst->num_buckets;
                cauchy_orset_entry_t* new_entry = cauchy_pool_alloc(dst->entry_pool);
                if (!new_entry) new_entry = malloc(sizeof(cauchy_orset_entry_t));
                if (!new_entry) return CAUCHY_ERR_NOMEM;

                new_entry->data = malloc(src_entry->size);
                if (!new_entry->data) {
                    cauchy_pool_free(dst->entry_pool, new_entry);
                    return CAUCHY_ERR_NOMEM;
                }

                memcpy(new_entry->data, src_entry->data, src_entry->size);
                new_entry->size = src_entry->size;
                new_entry->hash = src_entry->hash;
                new_entry->tag = src_entry->tag;
                new_entry->removed = src_entry->removed;
                new_entry->next = dst->buckets[idx];
                dst->buckets[idx] = new_entry;
                dst->entry_count++;
                if (!new_entry->removed) dst->active_count++;
            }
            src_entry = src_entry->next;
        }
    }
    return CAUCHY_OK;
}

bool cauchy_orset_equals(const cauchy_orset_t* a, const cauchy_orset_t* b) {
    if (!a || !b) return a == b;
    if (a->active_count != b->active_count) return false;

    for (usize i = 0; i < a->num_buckets; i++) {
        cauchy_orset_entry_t* entry = a->buckets[i];
        while (entry) {
            if (!entry->removed && !cauchy_orset_contains(b, entry->data, entry->size)) {
                return false;
            }
            entry = entry->next;
        }
    }
    return true;
}

void cauchy_orset_iter_init(cauchy_orset_iter_t* iter, const cauchy_orset_t* set) {
    if (!iter) return;
    iter->set = set;
    iter->bucket_idx = 0;
    iter->current = set ? set->buckets[0] : NULL;
    iter->last_data = NULL;
    iter->last_size = 0;

    while (set && iter->bucket_idx < set->num_buckets) {
        while (iter->current && iter->current->removed) {
            iter->current = iter->current->next;
        }
        if (iter->current) break;
        iter->bucket_idx++;
        if (iter->bucket_idx < set->num_buckets) {
            iter->current = set->buckets[iter->bucket_idx];
        }
    }
}

bool cauchy_orset_iter_next(cauchy_orset_iter_t* iter, const void** data, usize* size) {
    if (!iter || !iter->set) return false;

    while (iter->current || iter->bucket_idx < iter->set->num_buckets - 1) {
        if (iter->current && !iter->current->removed) {
            if (data) *data = iter->current->data;
            if (size) *size = iter->current->size;

            do {
                iter->current = iter->current->next;
            } while (iter->current && iter->current->removed);

            if (!iter->current) {
                while (++iter->bucket_idx < iter->set->num_buckets) {
                    iter->current = iter->set->buckets[iter->bucket_idx];
                    while (iter->current && iter->current->removed) {
                        iter->current = iter->current->next;
                    }
                    if (iter->current) break;
                }
            }
            return true;
        }
        if (++iter->bucket_idx < iter->set->num_buckets) {
            iter->current = iter->set->buckets[iter->bucket_idx];
        }
    }
    return false;
}

cauchy_result_t cauchy_orset_add_string(cauchy_orset_t* set, const char* str) {
    if (!str) return CAUCHY_ERR_INVALID;
    return cauchy_orset_add(set, str, strlen(str) + 1);
}

cauchy_result_t cauchy_orset_remove_string(cauchy_orset_t* set, const char* str) {
    if (!str) return CAUCHY_ERR_INVALID;
    return cauchy_orset_remove(set, str, strlen(str) + 1);
}

bool cauchy_orset_contains_string(const cauchy_orset_t* set, const char* str) {
    if (!str) return false;
    return cauchy_orset_contains(set, str, strlen(str) + 1);
}

void cauchy_orset_debug_print(const cauchy_orset_t* set, const char* label) {
    if (!set) { fprintf(stderr, "%s: (null)\n", label ? label : "orset"); return; }
    fprintf(stderr, "%s: entries=%zu active=%zu\n",
            label ? label : "orset", set->entry_count, set->active_count);
}
