/*
 * CAUCHY - Lock-Free Distributed State Convergence
 * Type Utilities Implementation
 */

#include "cauchy/types.h"

const char* cauchy_result_str(cauchy_result_t result) {
    switch (result) {
        case CAUCHY_OK:             return "OK";
        case CAUCHY_ERR_NOMEM:      return "Out of memory";
        case CAUCHY_ERR_INVALID:    return "Invalid argument";
        case CAUCHY_ERR_NOTFOUND:   return "Not found";
        case CAUCHY_ERR_EXISTS:     return "Already exists";
        case CAUCHY_ERR_FULL:       return "Container full";
        case CAUCHY_ERR_EMPTY:      return "Container empty";
        case CAUCHY_ERR_TIMEOUT:    return "Operation timed out";
        case CAUCHY_ERR_CONCURRENT: return "Concurrent modification";
        case CAUCHY_ERR_CAUSAL:     return "Causal dependency not satisfied";
        case CAUCHY_ERR_NETWORK:    return "Network error";
        case CAUCHY_ERR_INTERNAL:   return "Internal error";
        default:                    return "Unknown error";
    }
}

const char* cauchy_crdt_type_str(cauchy_crdt_type_t type) {
    switch (type) {
        case CAUCHY_CRDT_G_COUNTER:    return "G-Counter";
        case CAUCHY_CRDT_PN_COUNTER:   return "PN-Counter";
        case CAUCHY_CRDT_LWW_REGISTER: return "LWW-Register";
        case CAUCHY_CRDT_G_SET:        return "G-Set";
        case CAUCHY_CRDT_2P_SET:       return "2P-Set";
        case CAUCHY_CRDT_OR_SET:       return "OR-Set";
        case CAUCHY_CRDT_LWW_MAP:      return "LWW-Map";
        case CAUCHY_CRDT_RGA:          return "RGA";
        default:                       return "Unknown CRDT";
    }
}

