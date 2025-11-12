#ifndef COUNT_MIN_SKETCH_H
#define COUNT_MIN_SKETCH_H

#include <limits.h>  // per UINT_MAX
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LONG_PRIME 4294967311UL  // usato per migliorare la distribuzione degli hash
const double EPSILON = 0.1;      // should set this to 0.01 but with 0.1 the matrix is smaller, which is better for debugging
const double DELTA = 0.1;
const uint32_t PRIME = 2147483647;  // Mersenne's prime

typedef struct {
  uint32_t a;
  uint32_t b;
  uint32_t prime;
  uint32_t width;
} UniversalHash;

typedef struct {
  uint32_t** table;  // array contatori depth x width
  uint32_t depth;    // depth
  uint32_t width;    // width
  uint32_t total;    // totale conteggi
  double epsilon;
  double delta;
  UniversalHash* hashFunctions;
} CountMinSketch;

// update per un item intero
void cms_update_int(CountMinSketch* cms, uint32_t item, uint32_t c);

// hash semplice per stringa (djb2)
unsigned int cms_hashstr(const char* str);

// update per stringa
void cms_update_str(CountMinSketch* cms, const char* str, uint32_t c);

// point query per intero
unsigned int cms_point_query_int(CountMinSketch* cms, uint32_t item);

// point query per stringa
unsigned int cms_point_query_str(CountMinSketch* cms, const char* str);

// range query per intero
unsigned int cms_range_query_int(CountMinSketch* cms, int start, int end);

// range query per stringa
unsigned int cms_range_query_str(CountMinSketch* cms, const char** items, int n);

uint32_t cms_init(CountMinSketch* cms, double epsilon, double delta, uint32_t prime);
void cms_print(const CountMinSketch* cms);
void universal_hash_array_init(UniversalHash* hash, uint32_t prime, uint32_t width, uint32_t depth);
void universal_hash_init(UniversalHash* hash, uint32_t prime, uint32_t width);
void universal_hash_print(const UniversalHash* hash);
uint32_t hash_val(uint32_t val, const UniversalHash* hash);

#endif  // COUNT_MIN_SKETCH_H
