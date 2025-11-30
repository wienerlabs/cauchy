/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * LWW-Register Implementation
 */

#include "cauchy/crdt/lww_register.h"
#include <string.h>
#include <stdio.h>

void cauchy_lww_init(cauchy_lww_register_t* reg) {
    if (!reg) return;
    memset(reg, 0, sizeof(cauchy_lww_register_t));
}

cauchy_lww_register_t* cauchy_lww_create(void) {
    cauchy_lww_register_t* reg = cauchy_aligned_alloc(
        sizeof(cauchy_lww_register_t), CAUCHY_CACHE_LINE_SIZE);
    if (reg) cauchy_lww_init(reg);
    return reg;
}

void cauchy_lww_destroy(cauchy_lww_register_t* reg) {
    cauchy_aligned_free(reg);
}

cauchy_result_t cauchy_lww_set(cauchy_lww_register_t* reg,
                               const void* value,
                               usize value_size,
                               cauchy_timestamp_t timestamp,
                               cauchy_node_id_t node_id) {
    if (!reg) return CAUCHY_ERR_INVALID;
    if (value_size > CAUCHY_LWW_MAX_VALUE_SIZE) return CAUCHY_ERR_FULL;
    
    if (timestamp > reg->timestamp ||
        (timestamp == reg->timestamp && node_id > reg->node_id)) {
        if (value && value_size > 0) {
            memcpy(reg->value, value, value_size);
        }
        reg->value_size = value_size;
        reg->timestamp = timestamp;
        reg->node_id = node_id;
    }
    return CAUCHY_OK;
}

const void* cauchy_lww_get(const cauchy_lww_register_t* reg, usize* out_size) {
    if (!reg || reg->value_size == 0) {
        if (out_size) *out_size = 0;
        return NULL;
    }
    if (out_size) *out_size = reg->value_size;
    return reg->value;
}

cauchy_timestamp_t cauchy_lww_timestamp(const cauchy_lww_register_t* reg) {
    return reg ? reg->timestamp : 0;
}

bool cauchy_lww_has_value(const cauchy_lww_register_t* reg) {
    return reg && reg->value_size > 0;
}

void cauchy_lww_merge(cauchy_lww_register_t* dst, const cauchy_lww_register_t* src) {
    if (!dst || !src) return;
    if (src->timestamp > dst->timestamp ||
        (src->timestamp == dst->timestamp && src->node_id > dst->node_id)) {
        cauchy_lww_copy(dst, src);
    }
}

bool cauchy_lww_equals(const cauchy_lww_register_t* a, const cauchy_lww_register_t* b) {
    if (!a || !b) return a == b;
    if (a->timestamp != b->timestamp) return false;
    if (a->node_id != b->node_id) return false;
    if (a->value_size != b->value_size) return false;
    return memcmp(a->value, b->value, a->value_size) == 0;
}

void cauchy_lww_copy(cauchy_lww_register_t* dst, const cauchy_lww_register_t* src) {
    if (!dst || !src) return;
    memcpy(dst, src, sizeof(cauchy_lww_register_t));
}

cauchy_lww_register_t* cauchy_lww_clone(const cauchy_lww_register_t* reg) {
    if (!reg) return NULL;
    cauchy_lww_register_t* clone = cauchy_lww_create();
    if (clone) cauchy_lww_copy(clone, reg);
    return clone;
}

usize cauchy_lww_serialized_size(const cauchy_lww_register_t* reg) {
    if (!reg) return 0;
    return sizeof(cauchy_timestamp_t) + sizeof(cauchy_node_id_t) + 
           sizeof(usize) + reg->value_size;
}

usize cauchy_lww_serialize(const cauchy_lww_register_t* reg, u8* buffer, usize size) {
    if (!reg || !buffer) return 0;
    usize needed = cauchy_lww_serialized_size(reg);
    if (size < needed) return 0;
    
    usize offset = 0;
    memcpy(buffer + offset, &reg->timestamp, sizeof(reg->timestamp)); offset += sizeof(reg->timestamp);
    memcpy(buffer + offset, &reg->node_id, sizeof(reg->node_id)); offset += sizeof(reg->node_id);
    memcpy(buffer + offset, &reg->value_size, sizeof(reg->value_size)); offset += sizeof(reg->value_size);
    if (reg->value_size > 0) {
        memcpy(buffer + offset, reg->value, reg->value_size);
    }
    return needed;
}

cauchy_result_t cauchy_lww_deserialize(cauchy_lww_register_t* reg,
                                        const u8* buffer, usize size) {
    if (!reg || !buffer) return CAUCHY_ERR_INVALID;
    usize min_size = sizeof(cauchy_timestamp_t) + sizeof(cauchy_node_id_t) + sizeof(usize);
    if (size < min_size) return CAUCHY_ERR_INVALID;
    
    usize offset = 0;
    memcpy(&reg->timestamp, buffer + offset, sizeof(reg->timestamp)); offset += sizeof(reg->timestamp);
    memcpy(&reg->node_id, buffer + offset, sizeof(reg->node_id)); offset += sizeof(reg->node_id);
    memcpy(&reg->value_size, buffer + offset, sizeof(reg->value_size)); offset += sizeof(reg->value_size);
    
    if (reg->value_size > CAUCHY_LWW_MAX_VALUE_SIZE) return CAUCHY_ERR_INVALID;
    if (size < offset + reg->value_size) return CAUCHY_ERR_INVALID;
    if (reg->value_size > 0) {
        memcpy(reg->value, buffer + offset, reg->value_size);
    }
    return CAUCHY_OK;
}

cauchy_result_t cauchy_lww_set_u64(cauchy_lww_register_t* reg, u64 value,
                                   cauchy_timestamp_t ts, cauchy_node_id_t node) {
    return cauchy_lww_set(reg, &value, sizeof(value), ts, node);
}

u64 cauchy_lww_get_u64(const cauchy_lww_register_t* reg) {
    usize size;
    const void* val = cauchy_lww_get(reg, &size);
    if (!val || size != sizeof(u64)) return 0;
    u64 result;
    memcpy(&result, val, sizeof(result));
    return result;
}

cauchy_result_t cauchy_lww_set_string(cauchy_lww_register_t* reg, const char* value,
                                      cauchy_timestamp_t ts, cauchy_node_id_t node) {
    return cauchy_lww_set(reg, value, value ? strlen(value) + 1 : 0, ts, node);
}

const char* cauchy_lww_get_string(const cauchy_lww_register_t* reg) {
    usize size;
    const void* val = cauchy_lww_get(reg, &size);
    return (const char*)val;
}

void cauchy_lww_debug_print(const cauchy_lww_register_t* reg, const char* label) {
    if (!reg) { fprintf(stderr, "%s: (null)\n", label ? label : "lww"); return; }
    fprintf(stderr, "%s: ts=%llu node=%llu size=%zu\n",
            label ? label : "lww",
            (unsigned long long)reg->timestamp,
            (unsigned long long)reg->node_id,
            reg->value_size);
}

