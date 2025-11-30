/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * LWW-Register (Last-Write-Wins Register) CRDT
 * 
 * Simple register that resolves conflicts by timestamp.
 * Later timestamp always wins. Requires loosely synchronized clocks.
 */

#ifndef CAUCHY_CRDT_LWW_REGISTER_H
#define CAUCHY_CRDT_LWW_REGISTER_H

#include "../types.h"
#include "../memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum value size in bytes */
#ifndef CAUCHY_LWW_MAX_VALUE_SIZE
#define CAUCHY_LWW_MAX_VALUE_SIZE 256
#endif

/* LWW-Register structure */
typedef struct CAUCHY_CACHE_ALIGNED cauchy_lww_register {
    u8                  value[CAUCHY_LWW_MAX_VALUE_SIZE];
    usize               value_size;
    cauchy_timestamp_t  timestamp;
    cauchy_node_id_t    node_id;  /* Tie-breaker when timestamps equal */
} cauchy_lww_register_t;

/* Initialize an empty LWW-Register */
void cauchy_lww_init(cauchy_lww_register_t* reg);

/* Create a new LWW-Register on heap */
cauchy_lww_register_t* cauchy_lww_create(void);

/* Destroy a heap-allocated LWW-Register */
void cauchy_lww_destroy(cauchy_lww_register_t* reg);

/* Set the value with a timestamp */
cauchy_result_t cauchy_lww_set(cauchy_lww_register_t* reg,
                               const void* value,
                               usize value_size,
                               cauchy_timestamp_t timestamp,
                               cauchy_node_id_t node_id);

/* Get the current value */
const void* cauchy_lww_get(const cauchy_lww_register_t* reg, usize* out_size);

/* Get the timestamp */
cauchy_timestamp_t cauchy_lww_timestamp(const cauchy_lww_register_t* reg);

/* Check if register has a value */
bool cauchy_lww_has_value(const cauchy_lww_register_t* reg);

/* Merge another register (last-write-wins) */
void cauchy_lww_merge(cauchy_lww_register_t* dst, const cauchy_lww_register_t* src);

/* Check equality */
bool cauchy_lww_equals(const cauchy_lww_register_t* a, const cauchy_lww_register_t* b);

/* Copy state */
void cauchy_lww_copy(cauchy_lww_register_t* dst, const cauchy_lww_register_t* src);

/* Clone */
cauchy_lww_register_t* cauchy_lww_clone(const cauchy_lww_register_t* reg);

/* Serialization */
usize cauchy_lww_serialized_size(const cauchy_lww_register_t* reg);
usize cauchy_lww_serialize(const cauchy_lww_register_t* reg, u8* buffer, usize size);
cauchy_result_t cauchy_lww_deserialize(cauchy_lww_register_t* reg,
                                        const u8* buffer, usize size);

/* Debug output */
void cauchy_lww_debug_print(const cauchy_lww_register_t* reg, const char* label);

/* Convenience functions for common types */
cauchy_result_t cauchy_lww_set_u64(cauchy_lww_register_t* reg, u64 value,
                                   cauchy_timestamp_t ts, cauchy_node_id_t node);
u64 cauchy_lww_get_u64(const cauchy_lww_register_t* reg);

cauchy_result_t cauchy_lww_set_string(cauchy_lww_register_t* reg, const char* value,
                                      cauchy_timestamp_t ts, cauchy_node_id_t node);
const char* cauchy_lww_get_string(const cauchy_lww_register_t* reg);

#ifdef __cplusplus
}
#endif

#endif /* CAUCHY_CRDT_LWW_REGISTER_H */

