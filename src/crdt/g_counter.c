/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * G-Counter Implementation
 */

#include "cauchy/crdt/g_counter.h"
#include <string.h>
#include <stdio.h>

void cauchy_gcounter_init(cauchy_gcounter_t* gc, u32 num_nodes) {
    if (!gc) return;
    memset(gc->counts, 0, sizeof(gc->counts));
    gc->num_nodes = (num_nodes <= CAUCHY_MAX_NODES) ? num_nodes : CAUCHY_MAX_NODES;
}

cauchy_gcounter_t* cauchy_gcounter_create(u32 num_nodes) {
    cauchy_gcounter_t* gc = cauchy_aligned_alloc(
        sizeof(cauchy_gcounter_t), CAUCHY_CACHE_LINE_SIZE);
    if (gc) {
        cauchy_gcounter_init(gc, num_nodes);
    }
    return gc;
}

void cauchy_gcounter_destroy(cauchy_gcounter_t* gc) {
    cauchy_aligned_free(gc);
}

void cauchy_gcounter_increment(cauchy_gcounter_t* gc, cauchy_node_id_t node_id) {
    if (!gc || node_id >= gc->num_nodes) return;
    gc->counts[node_id]++;
}

void cauchy_gcounter_add(cauchy_gcounter_t* gc, cauchy_node_id_t node_id, u64 delta) {
    if (!gc || node_id >= gc->num_nodes) return;
    gc->counts[node_id] += delta;
}

u64 cauchy_gcounter_value(const cauchy_gcounter_t* gc) {
    if (!gc) return 0;
    u64 sum = 0;
    for (u32 i = 0; i < gc->num_nodes; i++) {
        sum += gc->counts[i];
    }
    return sum;
}

u64 cauchy_gcounter_get(const cauchy_gcounter_t* gc, cauchy_node_id_t node_id) {
    if (!gc || node_id >= gc->num_nodes) return 0;
    return gc->counts[node_id];
}

void cauchy_gcounter_merge(cauchy_gcounter_t* dst, const cauchy_gcounter_t* src) {
    if (!dst || !src) return;
    u32 max_nodes = (dst->num_nodes > src->num_nodes) ? dst->num_nodes : src->num_nodes;
    for (u32 i = 0; i < max_nodes; i++) {
        if (i < src->num_nodes && src->counts[i] > dst->counts[i]) {
            dst->counts[i] = src->counts[i];
        }
    }
    if (src->num_nodes > dst->num_nodes) {
        dst->num_nodes = src->num_nodes;
    }
}

bool cauchy_gcounter_equals(const cauchy_gcounter_t* a, const cauchy_gcounter_t* b) {
    if (!a || !b) return a == b;
    if (a->num_nodes != b->num_nodes) return false;
    for (u32 i = 0; i < a->num_nodes; i++) {
        if (a->counts[i] != b->counts[i]) return false;
    }
    return true;
}

cauchy_causality_t cauchy_gcounter_compare(const cauchy_gcounter_t* a,
                                            const cauchy_gcounter_t* b) {
    if (!a || !b) return CAUCHY_CONCURRENT;
    
    bool a_less = false, a_greater = false;
    u32 max_nodes = (a->num_nodes > b->num_nodes) ? a->num_nodes : b->num_nodes;
    
    for (u32 i = 0; i < max_nodes; i++) {
        u64 av = (i < a->num_nodes) ? a->counts[i] : 0;
        u64 bv = (i < b->num_nodes) ? b->counts[i] : 0;
        if (av < bv) a_less = true;
        if (av > bv) a_greater = true;
    }
    
    if (!a_less && !a_greater) return CAUCHY_EQUAL;
    if (a_less && !a_greater) return CAUCHY_HAPPENS_BEFORE;
    if (!a_less && a_greater) return CAUCHY_HAPPENS_AFTER;
    return CAUCHY_CONCURRENT;
}

void cauchy_gcounter_copy(cauchy_gcounter_t* dst, const cauchy_gcounter_t* src) {
    if (!dst || !src) return;
    memcpy(dst, src, sizeof(cauchy_gcounter_t));
}

cauchy_gcounter_t* cauchy_gcounter_clone(const cauchy_gcounter_t* gc) {
    if (!gc) return NULL;
    cauchy_gcounter_t* clone = cauchy_gcounter_create(gc->num_nodes);
    if (clone) cauchy_gcounter_copy(clone, gc);
    return clone;
}

usize cauchy_gcounter_serialized_size(const cauchy_gcounter_t* gc) {
    if (!gc) return 0;
    return sizeof(u32) + (gc->num_nodes * sizeof(u64));
}

usize cauchy_gcounter_serialize(const cauchy_gcounter_t* gc, u8* buffer, usize size) {
    if (!gc || !buffer) return 0;
    usize needed = cauchy_gcounter_serialized_size(gc);
    if (size < needed) return 0;
    memcpy(buffer, &gc->num_nodes, sizeof(u32));
    memcpy(buffer + sizeof(u32), gc->counts, gc->num_nodes * sizeof(u64));
    return needed;
}

cauchy_result_t cauchy_gcounter_deserialize(cauchy_gcounter_t* gc,
                                             const u8* buffer, usize size) {
    if (!gc || !buffer || size < sizeof(u32)) return CAUCHY_ERR_INVALID;
    u32 num_nodes;
    memcpy(&num_nodes, buffer, sizeof(u32));
    if (num_nodes > CAUCHY_MAX_NODES) return CAUCHY_ERR_INVALID;
    if (size < sizeof(u32) + num_nodes * sizeof(u64)) return CAUCHY_ERR_INVALID;
    cauchy_gcounter_init(gc, num_nodes);
    memcpy(gc->counts, buffer + sizeof(u32), num_nodes * sizeof(u64));
    return CAUCHY_OK;
}

void cauchy_gcounter_debug_print(const cauchy_gcounter_t* gc, const char* label) {
    if (!gc) { fprintf(stderr, "%s: (null)\n", label ? label : "gcounter"); return; }
    fprintf(stderr, "%s: value=%llu [", label ? label : "gcounter", 
            (unsigned long long)cauchy_gcounter_value(gc));
    for (u32 i = 0; i < gc->num_nodes; i++) {
        fprintf(stderr, "%llu%s", (unsigned long long)gc->counts[i], 
                (i < gc->num_nodes - 1) ? "," : "");
    }
    fprintf(stderr, "]\n");
}

