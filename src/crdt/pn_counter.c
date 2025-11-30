/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * PN-Counter Implementation
 */

#include "cauchy/crdt/pn_counter.h"
#include <string.h>
#include <stdio.h>

void cauchy_pncounter_init(cauchy_pncounter_t* pn, u32 num_nodes) {
    if (!pn) return;
    cauchy_gcounter_init(&pn->positive, num_nodes);
    cauchy_gcounter_init(&pn->negative, num_nodes);
}

cauchy_pncounter_t* cauchy_pncounter_create(u32 num_nodes) {
    cauchy_pncounter_t* pn = cauchy_aligned_alloc(
        sizeof(cauchy_pncounter_t), CAUCHY_CACHE_LINE_SIZE);
    if (pn) {
        cauchy_pncounter_init(pn, num_nodes);
    }
    return pn;
}

void cauchy_pncounter_destroy(cauchy_pncounter_t* pn) {
    cauchy_aligned_free(pn);
}

void cauchy_pncounter_increment(cauchy_pncounter_t* pn, cauchy_node_id_t node_id) {
    if (!pn) return;
    cauchy_gcounter_increment(&pn->positive, node_id);
}

void cauchy_pncounter_decrement(cauchy_pncounter_t* pn, cauchy_node_id_t node_id) {
    if (!pn) return;
    cauchy_gcounter_increment(&pn->negative, node_id);
}

void cauchy_pncounter_add(cauchy_pncounter_t* pn, cauchy_node_id_t node_id, i64 delta) {
    if (!pn) return;
    if (delta >= 0) {
        cauchy_gcounter_add(&pn->positive, node_id, (u64)delta);
    } else {
        cauchy_gcounter_add(&pn->negative, node_id, (u64)(-delta));
    }
}

i64 cauchy_pncounter_value(const cauchy_pncounter_t* pn) {
    if (!pn) return 0;
    return (i64)cauchy_gcounter_value(&pn->positive) - 
           (i64)cauchy_gcounter_value(&pn->negative);
}

u64 cauchy_pncounter_positive(const cauchy_pncounter_t* pn) {
    if (!pn) return 0;
    return cauchy_gcounter_value(&pn->positive);
}

u64 cauchy_pncounter_negative(const cauchy_pncounter_t* pn) {
    if (!pn) return 0;
    return cauchy_gcounter_value(&pn->negative);
}

void cauchy_pncounter_merge(cauchy_pncounter_t* dst, const cauchy_pncounter_t* src) {
    if (!dst || !src) return;
    cauchy_gcounter_merge(&dst->positive, &src->positive);
    cauchy_gcounter_merge(&dst->negative, &src->negative);
}

bool cauchy_pncounter_equals(const cauchy_pncounter_t* a, const cauchy_pncounter_t* b) {
    if (!a || !b) return a == b;
    return cauchy_gcounter_equals(&a->positive, &b->positive) &&
           cauchy_gcounter_equals(&a->negative, &b->negative);
}

void cauchy_pncounter_copy(cauchy_pncounter_t* dst, const cauchy_pncounter_t* src) {
    if (!dst || !src) return;
    cauchy_gcounter_copy(&dst->positive, &src->positive);
    cauchy_gcounter_copy(&dst->negative, &src->negative);
}

cauchy_pncounter_t* cauchy_pncounter_clone(const cauchy_pncounter_t* pn) {
    if (!pn) return NULL;
    cauchy_pncounter_t* clone = cauchy_pncounter_create(pn->positive.num_nodes);
    if (clone) cauchy_pncounter_copy(clone, pn);
    return clone;
}

usize cauchy_pncounter_serialized_size(const cauchy_pncounter_t* pn) {
    if (!pn) return 0;
    return cauchy_gcounter_serialized_size(&pn->positive) +
           cauchy_gcounter_serialized_size(&pn->negative);
}

usize cauchy_pncounter_serialize(const cauchy_pncounter_t* pn, u8* buffer, usize size) {
    if (!pn || !buffer) return 0;
    usize needed = cauchy_pncounter_serialized_size(pn);
    if (size < needed) return 0;
    
    usize pos_size = cauchy_gcounter_serialize(&pn->positive, buffer, size);
    if (pos_size == 0) return 0;
    
    usize neg_size = cauchy_gcounter_serialize(&pn->negative, 
                                                buffer + pos_size, size - pos_size);
    if (neg_size == 0) return 0;
    
    return pos_size + neg_size;
}

cauchy_result_t cauchy_pncounter_deserialize(cauchy_pncounter_t* pn,
                                              const u8* buffer, usize size) {
    if (!pn || !buffer || size < sizeof(u32) * 2) return CAUCHY_ERR_INVALID;
    
    cauchy_result_t res = cauchy_gcounter_deserialize(&pn->positive, buffer, size);
    if (res != CAUCHY_OK) return res;
    
    usize pos_size = cauchy_gcounter_serialized_size(&pn->positive);
    return cauchy_gcounter_deserialize(&pn->negative, buffer + pos_size, size - pos_size);
}

void cauchy_pncounter_debug_print(const cauchy_pncounter_t* pn, const char* label) {
    if (!pn) { fprintf(stderr, "%s: (null)\n", label ? label : "pncounter"); return; }
    fprintf(stderr, "%s: value=%lld (pos=%llu, neg=%llu)\n",
            label ? label : "pncounter",
            (long long)cauchy_pncounter_value(pn),
            (unsigned long long)cauchy_pncounter_positive(pn),
            (unsigned long long)cauchy_pncounter_negative(pn));
}

