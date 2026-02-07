#include "count_min_sketch_hybridV1.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>

// SERIAL CMS
uint32_t cms_init(CountMinSketch* cms, double epsilon, double delta, uint32_t prime) {
    cms->epsilon = epsilon;
    cms->delta = delta;
    cms->width = (uint32_t)ceil(exp(1.0)/epsilon);
    cms->depth = (uint32_t)ceil(log(1/delta));
    cms->total = 0;

    cms->table = malloc(cms->depth * sizeof(uint32_t*));
    for (uint32_t i = 0; i < cms->depth; i++)
        cms->table[i] = calloc(cms->width, sizeof(uint32_t));

    cms->hashFunctions = malloc(cms->depth * sizeof(UniversalHash));
    for (uint32_t i = 0; i < cms->depth; i++) {
        cms->hashFunctions[i].prime = prime;
        cms->hashFunctions[i].width = cms->width;
        cms->hashFunctions[i].a = rand() % (prime - 1) + 1;
        cms->hashFunctions[i].b = rand() % prime;
    }
    return 0;
}

void cms_free(CountMinSketch* cms) {
    if (!cms) return;
    if (cms->hashFunctions) free(cms->hashFunctions);
    if (cms->table) {
        for (uint32_t d = 0; d < cms->depth; d++)
            free(cms->table[d]);
        free(cms->table);
    }
    cms->hashFunctions = NULL;
    cms->table = NULL;
}

void cms_update_int(CountMinSketch* cms, uint32_t item, uint32_t count) {
    for (uint32_t d = 0; d < cms->depth; d++) {
        uint32_t idx = (cms->hashFunctions[d].a * item + cms->hashFunctions[d].b) % cms->hashFunctions[d].prime % cms->width;
        cms->table[d][idx] += count;
    }
    cms->total += count;
}

uint32_t cms_point_query_int(CountMinSketch* cms, uint32_t item) {
    uint32_t min_count = UINT_MAX;
    for (uint32_t d = 0; d < cms->depth; d++) {
        uint32_t idx = (cms->hashFunctions[d].a * item + cms->hashFunctions[d].b) % cms->hashFunctions[d].prime % cms->width;
        if (cms->table[d][idx] < min_count) min_count = cms->table[d][idx];
    }
    return min_count;
}

uint32_t cms_range_query_int(CountMinSketch* cms, int start, int end) {
    uint32_t total = 0;
    for (int i = start; i <= end; i++)
        total += cms_point_query_int(cms, i);
    return total;
}

uint32_t cms_inner_product(CountMinSketch* cms_a, CountMinSketch* cms_b) {
    if (cms_a->depth != cms_b->depth || cms_a->width != cms_b->width) return 0;
    uint32_t result = UINT_MAX;
    for (uint32_t d = 0; d < cms_a->depth; d++) {
        uint32_t sum = 0;
        for (uint32_t w = 0; w < cms_a->width; w++)
            sum += cms_a->table[d][w] * cms_b->table[d][w];
        if (sum < result) result = sum;
    }
    return result;
}

/* ---------- THREAD-PRIVATE CMS ---------- */
void cms_init_private(CountMinSketch* thread_cms, CountMinSketch* local_cms) {
    thread_cms->depth = local_cms->depth;
    thread_cms->width = local_cms->width;
    thread_cms->total = 0;
    thread_cms->epsilon = local_cms->epsilon;
    thread_cms->delta = local_cms->delta;

    thread_cms->hashFunctions = malloc(thread_cms->depth * sizeof(UniversalHash));
    for (uint32_t d = 0; d < thread_cms->depth; d++)
        thread_cms->hashFunctions[d] = local_cms->hashFunctions[d];

    thread_cms->table = malloc(thread_cms->depth * sizeof(uint32_t*));
    for (uint32_t d = 0; d < thread_cms->depth; d++)
        thread_cms->table[d] = calloc(thread_cms->width, sizeof(uint32_t));
}

void cms_update_int_parallel(CountMinSketch* cms, uint32_t item, uint32_t count) {
    cms_update_int(cms, item, count);
}

void cms_free_private(CountMinSketch* cms) {
    if (!cms) return;
    if (cms->hashFunctions) free(cms->hashFunctions);
    if (cms->table) {
        for (uint32_t d = 0; d < cms->depth; d++)
            free(cms->table[d]);
        free(cms->table);
    }
    cms->hashFunctions = NULL;
    cms->table = NULL;
}

/* ---------- TEST FUNCTIONS ---------- */
void test_basic_update_query(CountMinSketch* cms, uint32_t true_A, uint32_t true_B) {
    printf("Start Test: Basic Update and Query\n");
    uint32_t est_A = cms_point_query_int(cms, 123);
    uint32_t est_B = cms_point_query_int(cms, 456);
    uint32_t est_C = cms_point_query_int(cms, 999);
    printf("Item 123 → estimation: %u, real: %u\n", est_A, true_A);
    printf("Item 456 → estimation: %u, real: %u\n", est_B, true_B);
    printf("Item 999 → estimation: %u (expected: 0 or small)\n", est_C);
}

void test_range_query(CountMinSketch* cms, uint32_t true_range_sum) {
    printf("Start Test: Range Query\n");
    uint32_t est = cms_range_query_int(cms, 100, 110);
    printf("Range 100–110 → estimation: %u, real: %u\n", est, true_range_sum);
}

