// count_min_prova_parallel.c
#include "count_min_sketch_hybridV3.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <omp.h>

/* ---------- SERIAL CMS ---------- */
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

/* ---------- CMS UPDATE ---------- */
uint32_t hash_val(uint32_t val, const UniversalHash* hash) {
    return ((hash->a * val + hash->b) % hash->prime) % hash->width;
}

void cms_update_int_parallel(CountMinSketch* cms, uint32_t item, uint32_t count) {
    #pragma omp atomic
    cms->total += count;

    for (uint32_t j = 0; j < cms->depth; j++) {
        uint32_t hash_value = hash_val(item, &cms->hashFunctions[j]);
        #pragma omp atomic
        cms->table[j][hash_value] += count;
    }
}

uint32_t cms_point_query_int(CountMinSketch* cms, uint32_t item) {
    uint32_t min_count = UINT_MAX;
    for (uint32_t j = 0; j < cms->depth; j++) {
        uint32_t hash_value = hash_val(item, &cms->hashFunctions[j]);
        if (cms->table[j][hash_value] < min_count)
            min_count = cms->table[j][hash_value];
    }
    return min_count;
}

/* ---------- NUOVE FUNZIONI PARALLELE ---------- */
uint32_t cms_range_query_int_parallel(CountMinSketch* cms, int start, int end) {
    uint32_t total = 0;
    #pragma omp parallel for reduction(+:total)
    for (int i = start; i <= end; i++) {
        total += cms_point_query_int(cms, i);
    }
    return total;
}

uint32_t cms_inner_product_parallel(CountMinSketch* cms_a, CountMinSketch* cms_b) {
    if (cms_a->depth != cms_b->depth || cms_a->width != cms_b->width) return 0;

    uint32_t min_result = UINT_MAX;

    #pragma omp parallel
    {
        uint32_t local_min = UINT_MAX;

        #pragma omp for
        for (uint32_t d = 0; d < cms_a->depth; d++) {
            uint32_t row_dot = 0;
            for (uint32_t w = 0; w < cms_a->width; w++)
                row_dot += cms_a->table[d][w] * cms_b->table[d][w];
            if (row_dot < local_min)
                local_min = row_dot;
        }

        #pragma omp critical
        {
            if (local_min < min_result)
                min_result = local_min;
        }
    }

    return min_result;
}
