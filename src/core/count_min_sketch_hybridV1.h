#ifndef COUNT_MIN_SKETCH_HYBRIDV1_H
#define COUNT_MIN_SKETCH_HYBRIDV1_H

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// CONFIG 
#define EPSILON 0.001
#define DELTA 0.1
#define PRIME 2147483647
#define LONG_PRIME 4294967311UL

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

// DATA STRUCTURES 
typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t prime;
    uint32_t width;
} UniversalHash;

typedef struct {
    uint32_t** table;
    uint32_t depth;
    uint32_t width;
    uint32_t total;
    double epsilon;
    double delta;
    UniversalHash* hashFunctions;
} CountMinSketch;

typedef struct {
    uint32_t val;
    uint32_t count;
} RealCount;

// SERIAL CMS FUNCTIONS 
uint32_t cms_init(CountMinSketch* cms, double epsilon, double delta, uint32_t prime);
void cms_free(CountMinSketch* cms);
void cms_update_int(CountMinSketch* cms, uint32_t item, uint32_t count);
uint32_t cms_point_query_int(CountMinSketch* cms, uint32_t item);
uint32_t cms_range_query_int(CountMinSketch* cms, int start, int end);
uint32_t cms_inner_product(CountMinSketch* cms_a, CountMinSketch* cms_b);

// THREAD-PRIVATE CMS FUNCTIONS 
void cms_init_private(CountMinSketch* thread_cms, CountMinSketch* local_cms);
void cms_update_int_parallel(CountMinSketch* cms, uint32_t item, uint32_t count);
void cms_free_private(CountMinSketch* cms);

// TEST FUNCTIONS
void test_basic_update_query(CountMinSketch* cms, uint32_t true_A, uint32_t true_B);
void test_range_query(CountMinSketch* cms, uint32_t true_range_sum);

#endif // COUNT_MIN_SKETCH_HYBRID_H

