/*
 * CAUCHY - G-Counter Tests
 */

#include "cauchy/cauchy.h"
#include "cauchy/crdt/g_counter.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define TEST(name) static void test_##name(void)
#define RUN(name) do { printf("  %-40s", #name); test_##name(); printf("✓\n"); } while(0)

TEST(gcounter_init) {
    cauchy_gcounter_t gc;
    cauchy_gcounter_init(&gc, 3);
    assert(gc.num_nodes == 3);
    assert(cauchy_gcounter_value(&gc) == 0);
}

TEST(gcounter_increment) {
    cauchy_gcounter_t gc;
    cauchy_gcounter_init(&gc, 3);
    
    cauchy_gcounter_increment(&gc, 0);
    cauchy_gcounter_increment(&gc, 0);
    cauchy_gcounter_increment(&gc, 1);
    
    assert(cauchy_gcounter_get(&gc, 0) == 2);
    assert(cauchy_gcounter_get(&gc, 1) == 1);
    assert(cauchy_gcounter_get(&gc, 2) == 0);
    assert(cauchy_gcounter_value(&gc) == 3);
}

TEST(gcounter_merge_commutative) {
    cauchy_gcounter_t a, b;
    cauchy_gcounter_init(&a, 3);
    cauchy_gcounter_init(&b, 3);
    
    cauchy_gcounter_add(&a, 0, 5);
    cauchy_gcounter_add(&a, 1, 3);
    cauchy_gcounter_add(&b, 1, 7);
    cauchy_gcounter_add(&b, 2, 2);
    
    cauchy_gcounter_t ab, ba;
    cauchy_gcounter_copy(&ab, &a);
    cauchy_gcounter_copy(&ba, &b);
    
    cauchy_gcounter_merge(&ab, &b);
    cauchy_gcounter_merge(&ba, &a);
    
    assert(cauchy_gcounter_equals(&ab, &ba));
    assert(cauchy_gcounter_value(&ab) == 5 + 7 + 2);
}

TEST(gcounter_merge_associative) {
    cauchy_gcounter_t a, b, c;
    cauchy_gcounter_init(&a, 3);
    cauchy_gcounter_init(&b, 3);
    cauchy_gcounter_init(&c, 3);
    
    cauchy_gcounter_add(&a, 0, 5);
    cauchy_gcounter_add(&b, 1, 7);
    cauchy_gcounter_add(&c, 2, 3);
    
    cauchy_gcounter_t ab_c, a_bc;
    
    cauchy_gcounter_copy(&ab_c, &a);
    cauchy_gcounter_merge(&ab_c, &b);
    cauchy_gcounter_merge(&ab_c, &c);
    
    cauchy_gcounter_t bc;
    cauchy_gcounter_copy(&bc, &b);
    cauchy_gcounter_merge(&bc, &c);
    cauchy_gcounter_copy(&a_bc, &a);
    cauchy_gcounter_merge(&a_bc, &bc);
    
    assert(cauchy_gcounter_equals(&ab_c, &a_bc));
}

TEST(gcounter_merge_idempotent) {
    cauchy_gcounter_t a;
    cauchy_gcounter_init(&a, 3);
    cauchy_gcounter_add(&a, 0, 5);
    cauchy_gcounter_add(&a, 1, 3);
    
    cauchy_gcounter_t original;
    cauchy_gcounter_copy(&original, &a);
    
    cauchy_gcounter_merge(&a, &a);
    
    assert(cauchy_gcounter_equals(&a, &original));
}

TEST(gcounter_serialization) {
    cauchy_gcounter_t gc;
    cauchy_gcounter_init(&gc, 3);
    cauchy_gcounter_add(&gc, 0, 100);
    cauchy_gcounter_add(&gc, 1, 200);
    cauchy_gcounter_add(&gc, 2, 300);
    
    u8 buffer[256];
    usize size = cauchy_gcounter_serialize(&gc, buffer, sizeof(buffer));
    assert(size > 0);
    
    cauchy_gcounter_t restored;
    cauchy_result_t res = cauchy_gcounter_deserialize(&restored, buffer, size);
    assert(res == CAUCHY_OK);
    assert(cauchy_gcounter_equals(&gc, &restored));
}

TEST(gcounter_convergence) {
    cauchy_gcounter_t node0, node1, node2;
    cauchy_gcounter_init(&node0, 3);
    cauchy_gcounter_init(&node1, 3);
    cauchy_gcounter_init(&node2, 3);
    
    for (int i = 0; i < 100; i++) cauchy_gcounter_increment(&node0, 0);
    for (int i = 0; i < 50; i++) cauchy_gcounter_increment(&node1, 1);
    for (int i = 0; i < 75; i++) cauchy_gcounter_increment(&node2, 2);
    
    cauchy_gcounter_merge(&node0, &node1);
    cauchy_gcounter_merge(&node1, &node2);
    cauchy_gcounter_merge(&node2, &node0);
    cauchy_gcounter_merge(&node0, &node2);
    cauchy_gcounter_merge(&node1, &node0);
    
    assert(cauchy_gcounter_equals(&node0, &node1));
    assert(cauchy_gcounter_equals(&node1, &node2));
    assert(cauchy_gcounter_value(&node0) == 225);
}

int main(void) {
    printf("G-Counter Tests:\n");
    
    RUN(gcounter_init);
    RUN(gcounter_increment);
    RUN(gcounter_merge_commutative);
    RUN(gcounter_merge_associative);
    RUN(gcounter_merge_idempotent);
    RUN(gcounter_serialization);
    RUN(gcounter_convergence);
    
    printf("\nAll G-Counter tests passed!\n");
    return 0;
}

