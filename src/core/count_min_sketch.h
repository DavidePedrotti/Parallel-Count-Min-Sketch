#ifndef COUNT_MIN_SKETCH_H
#define COUNT_MIN_SKETCH_H

#include <limits.h>  // per UINT_MAX
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define EPSILON 0.001  // should set this to 0.01 but with 0.1 the matrix is smaller, which is better for debugging
#define DELTA 0.1
#define PRIME 2147483647         // Mersenne's prime
#define LONG_PRIME 4294967311UL  // used to improve the distribution of hashes

typedef struct {
  uint32_t a;
  uint32_t b;
  uint32_t prime;
  uint32_t width;
} UniversalHash;

typedef struct {
  uint32_t** table;  // array counters depth x width
  uint32_t depth;    // depth
  uint32_t width;    // width
  uint32_t total;    // total counts
  double epsilon;
  double delta;
  UniversalHash* hashFunctions;
} CountMinSketch;

// struct used to store the real count of items
typedef struct {
  uint32_t val;
  uint32_t count;
} RealCount;

// update for an item represented as an integer
void cms_update_int(CountMinSketch* cms, uint32_t item, uint32_t c);

// simple hash for string (djb2)
uint32_t cms_hashstr(const char* str);

// update for a string
void cms_update_str(CountMinSketch* cms, const char* str, uint32_t c);

// point query for an integer
uint32_t cms_point_query_int(CountMinSketch* cms, uint32_t item);

// point query for a string
uint32_t cms_point_query_str(CountMinSketch* cms, const char* str);

// range query for an integer
uint32_t cms_range_query_int(CountMinSketch* cms, int start, int end);

// range query for a string
uint32_t cms_range_query_str(CountMinSketch* cms, const char** items, int n);

// inner product query
uint32_t cms_inner_product(CountMinSketch* cms_a, CountMinSketch* cms_b);

// initialize cms struct
uint32_t cms_init(CountMinSketch* cms, double epsilon, double delta, uint32_t prime);

// free dynamically allocated memory
void cms_free(CountMinSketch* cms);

// initialize a single hash function
void universal_hash_init(UniversalHash* hash, uint32_t prime, uint32_t width);

// initialize an array of hash functions
void universal_hash_array_init(UniversalHash* hash, uint32_t prime, uint32_t width, uint32_t depth);

// return the hash value of uint32_t
uint32_t hash_val(uint32_t val, const UniversalHash* hash);

// pretty print CMS
void cms_print_values(const CountMinSketch* cms, const char* cms_name);
void cms_print_table(const CountMinSketch* cms, const char* cms_name);
void cms_print_hashes(const CountMinSketch* cms, const char* cms_name);
void cms_print_all(const CountMinSketch* cms, const char* cms_name);

// pretty print UniversalHash
void universal_hash_print(const UniversalHash* hash);

// store the real count of items into a RealCount*
RealCount* load_count(const char* filename, uint32_t n_values);

int test_cms_accuracy(CountMinSketch* cms, RealCount* ground_truth, uint32_t n_values, uint32_t dataset_size);

void test_basic_update_query(CountMinSketch* cms, uint32_t true_A, uint32_t true_B);
void test_range_query(CountMinSketch* cms, uint32_t true_range_sum);
void test_inner_product(CountMinSketch* cms_a, CountMinSketch* cms_b);

void test_basic_update_query_demo();
void test_range_query_demo();
void test_inner_product_demo();

uint32_t count_lines(const char* filename);

#endif  // COUNT_MIN_SKETCH_H