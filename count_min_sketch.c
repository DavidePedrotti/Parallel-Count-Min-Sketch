#include "count_min_sketch.h"

// update per un item intero
void cms_update_int(CountMinSketch* cms, uint32_t item, uint32_t c) {
  cms->total += c;
  for (uint32_t j = 0; j < cms->depth; j++) {
    uint32_t hash_value = hash_val(item, &cms->hashFunctions[j]);
    cms->table[j][hash_value] += c;
  }
}

// Convertire una stringa in un numero intero
uint32_t cms_hashstr(const char* str) {
  unsigned long hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return (uint32_t)(hash % LONG_PRIME);
}

// update per stringa
void cms_update_str(CountMinSketch* cms, const char* str, uint32_t c) {
  uint32_t hashval = cms_hashstr(str);
  cms_update_int(cms, hashval, c);
}

// point query per intero
uint32_t cms_point_query_int(CountMinSketch* cms, uint32_t item) {
  uint32_t min_count = UINT_MAX;  // si parte dal massimo valore possibile
  for (uint32_t j = 0; j < cms->depth; j++) {
    uint32_t hash_value = hash_val(item, &cms->hashFunctions[j]);
    if (cms->table[j][hash_value] < min_count) {
      min_count = cms->table[j][hash_value];
    }
  }
  return min_count;
}

// point query per stringa
uint32_t cms_point_query_str(CountMinSketch* cms, const char* str) {
  uint32_t hashval = cms_hashstr(str);
  return cms_point_query_int(cms, hashval);
}

uint32_t cms_range_query_int(CountMinSketch* cms, int start, int end) {
  uint32_t total = 0;
  for (int i = start; i <= end; i++) {
    total += cms_point_query_int(cms, i);
  }
  return total;
}

uint32_t cms_range_query_str(CountMinSketch* cms, const char** items, int n) {
  uint32_t total = 0;
  for (int i = 0; i < n; i++) {
    total += cms_point_query_str(cms, items[i]);
  }
  return total;
}

// inner product query
// computes row-wise dot product between two sketches, and returns the min of these row-wise dot products
uint32_t cms_inner_product(CountMinSketch* cms_a, CountMinSketch* cms_b) {
  if (cms_a->depth != cms_b->depth) {
    fprintf(stderr, "Error: the two sketches must have the same number of rows. Rows given: %d, %d\n", cms_a->depth, cms_b->depth);
    return -1;
  }
  if (cms_a->width != cms_b->width) {
    fprintf(stderr, "Error: the two sketches must have the same number of columns. Columns given: %d, %d\n", cms_a->width, cms_b->width);
    return -2;
  }
  uint32_t result = UINT_MAX;
  for (uint32_t i = 0; i < cms_a->depth; i++) {
    uint32_t row_dot_product = 0;
    for (uint32_t j = 0; j < cms_a->width; j++) {
      row_dot_product += (cms_a->table[i][j] * cms_b->table[i][j]);
    }
    result = min(result, row_dot_product);
  }
  return result;
}

// initialize cms creating a table and assigning a set of hash functions
uint32_t cms_init(CountMinSketch* cms, double epsilon, double delta, uint32_t prime) {
  if (epsilon <= 0.0 || epsilon >= 1.0) {
    fprintf(stderr, "Error: epsilon value must be between 0 and 1 (exclusive)\n");
    return -1;
  }
  if (delta <= 0.0 || delta >= 1.0) {
    fprintf(stderr, "Error: delta value must be between 0 and 1 (exclusive)\n");
    return -2;
  }
  cms->epsilon = epsilon;
  cms->delta = delta;
  cms->width = ceil(exp(1.0) / epsilon);
  cms->depth = ceil(log(1 / delta));
  cms->table = malloc(cms->depth * sizeof(uint32_t*));
  for (uint32_t i = 0; i < cms->depth; i++) {
    cms->table[i] = malloc(cms->width * sizeof(uint32_t));
    for (uint32_t j = 0; j < cms->width; j++) {
      cms->table[i][j] = 0;
    }
  }
  cms->hashFunctions = malloc(cms->depth * sizeof(UniversalHash));
  universal_hash_array_init(cms->hashFunctions, prime, cms->width, cms->depth);
  return 0;
}

void cms_free(CountMinSketch* cms) {
  for(uint32_t i; i < cms->depth; i++) {
    free(cms->table[i]);
  }
  free(cms->table);
  free(cms->hashFunctions);
}

// initialize UniversalHash
void universal_hash_init(UniversalHash* hash, uint32_t prime, uint32_t width) {
  hash->prime = prime;
  hash->width = width;
  hash->a = rand() % (prime - 1) + 1;
  hash->b = rand() % prime;
}

// initialize an array of UniversalHash
void universal_hash_array_init(UniversalHash* hashFunctions, uint32_t prime, uint32_t width, uint32_t depth) {
  for (uint32_t i = 0; i < depth; i++) {
    universal_hash_init(&hashFunctions[i], prime, width);
  }
}

// compute the hash of a value
uint32_t hash_val(uint32_t val, const UniversalHash* hash) {
  return ((hash->a * val + hash->b) % hash->prime) % hash->width;
}

// pretty print UniversalHash
void universal_hash_print(const UniversalHash* hash) {
  printf(
      "hash values: \n"
      "\t a: %d\n"
      "\t b: %d\n"
      "\t prime: %d\n"
      "\t width: %d\n",
      hash->a, hash->b, hash->prime, hash->width);
}

// pretty print CountMinSketch
void cms_print_all(const CountMinSketch* cms, const char* cms_name) {
  cms_print_values(cms, cms_name);
  cms_print_table(cms, cms_name);
  cms_print_hashes(cms, cms_name);
}

void cms_print_values(const CountMinSketch* cms, const char* cms_name) {
  printf("%s values:\n", cms_name);
  printf(
      "\tepsilon: %f\n"
      "\tdelta: %f\n"
      "\tdepth: %d\n"
      "\twidth: %d\n",
      cms->epsilon, cms->delta, cms->depth, cms->width);
}

void cms_print_table(const CountMinSketch* cms, const char* cms_name) {
  printf("%s table:\n", cms_name);
  for (uint32_t i = 0; i < cms->depth; i++) {
    for (uint32_t j = 0; j < cms->width; j++) {
      printf("%d ", cms->table[i][j]);
    }
    printf("\n");
  }
}

void cms_print_hashes(const CountMinSketch* cms, const char* cms_name) {
  printf("%s hashes:\n", cms_name);
  for (uint32_t i = 0; i < cms->depth; i++) {
    universal_hash_print(&cms->hashFunctions[i]);
  }
}

// function to test the inner product between two CMS
// this function will be refactored once the code will be properly tested
void test_inner_product() {
  CountMinSketch cms_a;
  cms_init(&cms_a, 0.1, 0.1, 2147483647);  // hardcoding them to set depth=3 and width=28
  cms_a.table[0][0] = 1;
  cms_a.table[0][27] = 1;
  cms_a.table[1][0] = 2;
  cms_a.table[1][27] = 2;
  cms_a.table[2][0] = 3;
  cms_a.table[2][27] = 3;
  cms_print_table(&cms_a, "cms_a");

  CountMinSketch cms_b;
  cms_init(&cms_b, 0.1, 0.1, 2147483647);
  cms_b.table[0][0] = 2;
  cms_b.table[0][27] = 2;
  cms_b.table[1][0] = 2;
  cms_b.table[1][27] = 2;
  cms_b.table[2][0] = 3;
  cms_b.table[2][27] = 3;
  cms_print_table(&cms_b, "cms_b");

  uint32_t inner_product = cms_inner_product(&cms_a, &cms_b);
  printf("Inner product query: %d\n", inner_product);  // should be 4

  cms_free(&cms_a);
  cms_free(&cms_b);
}

int main(int argc, char* argv[]) {
  test_inner_product();
  return 0;
}