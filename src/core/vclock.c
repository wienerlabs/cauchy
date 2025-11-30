/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * Vector Clock Implementation
 */

#include "cauchy/vclock.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void cauchy_vclock_init(cauchy_vclock_t* vc, u32 num_nodes) {
    if (!vc) return;
    memset(vc->entries, 0, sizeof(vc->entries));
    vc->num_nodes = (num_nodes <= CAUCHY_MAX_NODES) ? num_nodes : CAUCHY_MAX_NODES;
}

cauchy_vclock_t* cauchy_vclock_create(u32 num_nodes) {
    cauchy_vclock_t* vc = cauchy_aligned_alloc(sizeof(cauchy_vclock_t), CAUCHY_CACHE_LINE_SIZE);
    if (vc) {
        cauchy_vclock_init(vc, num_nodes);
    }
    return vc;
}

void cauchy_vclock_destroy(cauchy_vclock_t* vc) {
    cauchy_aligned_free(vc);
}

void cauchy_vclock_copy(cauchy_vclock_t* dst, const cauchy_vclock_t* src) {
    if (!dst || !src) return;
    memcpy(dst, src, sizeof(cauchy_vclock_t));
}

cauchy_vclock_t* cauchy_vclock_clone(const cauchy_vclock_t* vc) {
    if (!vc) return NULL;
    cauchy_vclock_t* clone = cauchy_vclock_create(vc->num_nodes);
    if (clone) {
        cauchy_vclock_copy(clone, vc);
    }
    return clone;
}

void cauchy_vclock_increment(cauchy_vclock_t* vc, cauchy_node_id_t node_id) {
    if (!vc || node_id >= vc->num_nodes) return;
    vc->entries[node_id]++;
}

u64 cauchy_vclock_get(const cauchy_vclock_t* vc, cauchy_node_id_t node_id) {
    if (!vc || node_id >= vc->num_nodes) return 0;
    return vc->entries[node_id];
}

void cauchy_vclock_set(cauchy_vclock_t* vc, cauchy_node_id_t node_id, u64 value) {
    if (!vc || node_id >= vc->num_nodes) return;
    vc->entries[node_id] = value;
}

void cauchy_vclock_merge(cauchy_vclock_t* dst, const cauchy_vclock_t* src) {
    if (!dst || !src) return;
    u32 max_nodes = (dst->num_nodes > src->num_nodes) ? dst->num_nodes : src->num_nodes;
    for (u32 i = 0; i < max_nodes; i++) {
        if (src->entries[i] > dst->entries[i]) {
            dst->entries[i] = src->entries[i];
        }
    }
    if (src->num_nodes > dst->num_nodes) {
        dst->num_nodes = src->num_nodes;
    }
}

cauchy_causality_t cauchy_vclock_compare(const cauchy_vclock_t* a, const cauchy_vclock_t* b) {
    if (!a || !b) return CAUCHY_CONCURRENT;
    
    bool a_less = false, a_greater = false;
    u32 max_nodes = (a->num_nodes > b->num_nodes) ? a->num_nodes : b->num_nodes;
    
    for (u32 i = 0; i < max_nodes; i++) {
        u64 av = (i < a->num_nodes) ? a->entries[i] : 0;
        u64 bv = (i < b->num_nodes) ? b->entries[i] : 0;
        if (av < bv) a_less = true;
        if (av > bv) a_greater = true;
    }
    
    if (!a_less && !a_greater) return CAUCHY_EQUAL;
    if (a_less && !a_greater) return CAUCHY_HAPPENS_BEFORE;
    if (!a_less && a_greater) return CAUCHY_HAPPENS_AFTER;
    return CAUCHY_CONCURRENT;
}

bool cauchy_vclock_happens_before(const cauchy_vclock_t* a, const cauchy_vclock_t* b) {
    return cauchy_vclock_compare(a, b) == CAUCHY_HAPPENS_BEFORE;
}

bool cauchy_vclock_concurrent(const cauchy_vclock_t* a, const cauchy_vclock_t* b) {
    return cauchy_vclock_compare(a, b) == CAUCHY_CONCURRENT;
}

bool cauchy_vclock_equals(const cauchy_vclock_t* a, const cauchy_vclock_t* b) {
    return cauchy_vclock_compare(a, b) == CAUCHY_EQUAL;
}

bool cauchy_vclock_is_empty(const cauchy_vclock_t* vc) {
    if (!vc) return true;
    for (u32 i = 0; i < vc->num_nodes; i++) {
        if (vc->entries[i] != 0) return false;
    }
    return true;
}

u64 cauchy_vclock_sum(const cauchy_vclock_t* vc) {
    if (!vc) return 0;
    u64 sum = 0;
    for (u32 i = 0; i < vc->num_nodes; i++) {
        sum += vc->entries[i];
    }
    return sum;
}

void cauchy_vclock_min(cauchy_vclock_t* dst, const cauchy_vclock_t* src) {
    if (!dst || !src) return;
    for (u32 i = 0; i < dst->num_nodes && i < src->num_nodes; i++) {
        if (src->entries[i] < dst->entries[i]) {
            dst->entries[i] = src->entries[i];
        }
    }
}

usize cauchy_vclock_serialized_size(const cauchy_vclock_t* vc) {
    if (!vc) return 0;
    return sizeof(u32) + (vc->num_nodes * sizeof(u64));
}

usize cauchy_vclock_serialize(const cauchy_vclock_t* vc, u8* buffer, usize buffer_size) {
    if (!vc || !buffer) return 0;
    usize needed = cauchy_vclock_serialized_size(vc);
    if (buffer_size < needed) return 0;
    
    memcpy(buffer, &vc->num_nodes, sizeof(u32));
    memcpy(buffer + sizeof(u32), vc->entries, vc->num_nodes * sizeof(u64));
    return needed;
}

cauchy_result_t cauchy_vclock_deserialize(cauchy_vclock_t* vc, const u8* buffer, usize buffer_size) {
    if (!vc || !buffer || buffer_size < sizeof(u32)) return CAUCHY_ERR_INVALID;
    
    u32 num_nodes;
    memcpy(&num_nodes, buffer, sizeof(u32));
    if (num_nodes > CAUCHY_MAX_NODES) return CAUCHY_ERR_INVALID;
    if (buffer_size < sizeof(u32) + num_nodes * sizeof(u64)) return CAUCHY_ERR_INVALID;
    
    cauchy_vclock_init(vc, num_nodes);
    memcpy(vc->entries, buffer + sizeof(u32), num_nodes * sizeof(u64));
    return CAUCHY_OK;
}

void cauchy_vclock_debug_print(const cauchy_vclock_t* vc, const char* label) {
    if (!vc) { fprintf(stderr, "%s: (null)\n", label ? label : "vclock"); return; }
    fprintf(stderr, "%s: [", label ? label : "vclock");
    for (u32 i = 0; i < vc->num_nodes; i++) {
        fprintf(stderr, "%llu%s", (unsigned long long)vc->entries[i], (i < vc->num_nodes - 1) ? "," : "");
    }
    fprintf(stderr, "]\n");
}

