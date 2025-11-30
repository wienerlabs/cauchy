/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * 2P-Set Implementation
 */

#include "cauchy/crdt/2p_set.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

cauchy_result_t cauchy_2pset_init(cauchy_2pset_t* set, usize initial_capacity) {
    if (!set) return CAUCHY_ERR_INVALID;
    
    set->added = cauchy_gset_create(initial_capacity);
    if (!set->added) return CAUCHY_ERR_NOMEM;
    
    set->removed = cauchy_gset_create(initial_capacity);
    if (!set->removed) {
        cauchy_gset_destroy(set->added);
        return CAUCHY_ERR_NOMEM;
    }
    return CAUCHY_OK;
}

cauchy_2pset_t* cauchy_2pset_create(usize initial_capacity) {
    cauchy_2pset_t* set = malloc(sizeof(cauchy_2pset_t));
    if (!set) return NULL;
    if (cauchy_2pset_init(set, initial_capacity) != CAUCHY_OK) {
        free(set);
        return NULL;
    }
    return set;
}

void cauchy_2pset_destroy(cauchy_2pset_t* set) {
    if (!set) return;
    if (set->added) cauchy_gset_destroy(set->added);
    if (set->removed) cauchy_gset_destroy(set->removed);
    free(set);
}

cauchy_result_t cauchy_2pset_add(cauchy_2pset_t* set, const void* data, usize size) {
    if (!set) return CAUCHY_ERR_INVALID;
    if (cauchy_gset_contains(set->removed, data, size)) {
        return CAUCHY_OK;  /* Can't re-add removed element */
    }
    return cauchy_gset_add(set->added, data, size);
}

cauchy_result_t cauchy_2pset_remove(cauchy_2pset_t* set, const void* data, usize size) {
    if (!set) return CAUCHY_ERR_INVALID;
    if (!cauchy_gset_contains(set->added, data, size)) {
        return CAUCHY_ERR_NOTFOUND;  /* Can only remove if added */
    }
    return cauchy_gset_add(set->removed, data, size);
}

bool cauchy_2pset_contains(const cauchy_2pset_t* set, const void* data, usize size) {
    if (!set) return false;
    return cauchy_gset_contains(set->added, data, size) &&
           !cauchy_gset_contains(set->removed, data, size);
}

bool cauchy_2pset_was_added(const cauchy_2pset_t* set, const void* data, usize size) {
    if (!set) return false;
    return cauchy_gset_contains(set->added, data, size);
}

bool cauchy_2pset_was_removed(const cauchy_2pset_t* set, const void* data, usize size) {
    if (!set) return false;
    return cauchy_gset_contains(set->removed, data, size);
}

usize cauchy_2pset_count(const cauchy_2pset_t* set) {
    if (!set) return 0;
    usize count = 0;
    cauchy_gset_iter_t iter;
    cauchy_gset_iter_init(&iter, set->added);
    const void* data;
    usize size;
    while (cauchy_gset_iter_next(&iter, &data, &size)) {
        if (!cauchy_gset_contains(set->removed, data, size)) {
            count++;
        }
    }
    return count;
}

bool cauchy_2pset_is_empty(const cauchy_2pset_t* set) {
    return cauchy_2pset_count(set) == 0;
}

cauchy_result_t cauchy_2pset_merge(cauchy_2pset_t* dst, const cauchy_2pset_t* src) {
    if (!dst || !src) return CAUCHY_ERR_INVALID;
    
    cauchy_result_t res = cauchy_gset_merge(dst->added, src->added);
    if (res != CAUCHY_OK) return res;
    
    return cauchy_gset_merge(dst->removed, src->removed);
}

bool cauchy_2pset_equals(const cauchy_2pset_t* a, const cauchy_2pset_t* b) {
    if (!a || !b) return a == b;
    return cauchy_gset_equals(a->added, b->added) &&
           cauchy_gset_equals(a->removed, b->removed);
}

cauchy_result_t cauchy_2pset_add_string(cauchy_2pset_t* set, const char* str) {
    if (!str) return CAUCHY_ERR_INVALID;
    return cauchy_2pset_add(set, str, strlen(str) + 1);
}

cauchy_result_t cauchy_2pset_remove_string(cauchy_2pset_t* set, const char* str) {
    if (!str) return CAUCHY_ERR_INVALID;
    return cauchy_2pset_remove(set, str, strlen(str) + 1);
}

bool cauchy_2pset_contains_string(const cauchy_2pset_t* set, const char* str) {
    if (!str) return false;
    return cauchy_2pset_contains(set, str, strlen(str) + 1);
}

void cauchy_2pset_debug_print(const cauchy_2pset_t* set, const char* label) {
    if (!set) { 
        fprintf(stderr, "%s: (null)\n", label ? label : "2pset"); 
        return; 
    }
    fprintf(stderr, "%s: added=%zu removed=%zu active=%zu\n",
            label ? label : "2pset",
            cauchy_gset_count(set->added),
            cauchy_gset_count(set->removed),
            cauchy_2pset_count(set));
}

