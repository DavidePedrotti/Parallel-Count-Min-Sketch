#include "count_min_sketch.h"
#include <inttypes.h>

// update for an item represented as an integer
void cms_update_int(CountMinSketch* cms, uint32_t item, uint32_t c) {
  cms->total += c;
  for (uint32_t j = 0; j < cms->depth; j++) {
    uint32_t hash_value = hash_val(item, &cms->hashFunctions[j]);
    cms->table[j][hash_value] += c;
  }
}

// Convert a string to an integer
uint32_t cms_hashstr(const char* str) {
  unsigned long hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return (uint32_t)(hash % LONG_PRIME);
}

// update for a string
void cms_update_str(CountMinSketch* cms, const char* str, uint32_t c) {
  uint32_t hashval = cms_hashstr(str);
  cms_update_int(cms, hashval, c);
}

// point query for an integer
uint32_t cms_point_query_int(CountMinSketch* cms, uint32_t item) {
  uint32_t min_count = UINT_MAX;  // start from the maximum possible value
  for (uint32_t j = 0; j < cms->depth; j++) {
    uint32_t hash_value = hash_val(item, &cms->hashFunctions[j]);
    if (cms->table[j][hash_value] < min_count) {
      min_count = cms->table[j][hash_value];
    }
  }
  return min_count;
}

// point query for a string
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
  cms->total = 0;
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
  for (uint32_t i = 0; i < cms->depth; i++) {
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

// stores true item count into an array
RealCount* load_count(const char* filename, uint32_t n_values) {
  FILE* fp = fopen(filename, "r");
  if (!fp)
    return NULL;

  RealCount* arr = malloc(n_values * sizeof(RealCount));

  char line[100];
  uint32_t i = 0;
  while (fgets(line, sizeof(line), fp)) {
    sscanf(line, "%u %u", &arr[i].val, &arr[i].count);
    i++;
  }
  fclose(fp);

  return arr;
}

// tests cms accuracy vs the ground truth
int test_cms_accuracy(CountMinSketch* cms, RealCount* ground_truth, uint32_t n_values, uint32_t dataset_size) {
  printf("\nCMS Accuracy Evaluation\n");
  printf("Dataset size: %u\n", dataset_size);
  printf("Number of unique values: %u\n", n_values);
  cms_print_values(cms, "CMS");
  printf("Theoretical error bound: epsilon*N = %0.0f\n\n", cms->epsilon * dataset_size);

  uint64_t total_abs_error = 0;
  uint64_t max_abs_error = 0;
  uint64_t total_exact_matches = 0;
  uint64_t total_within_bound = 0;
  double error_bound = cms->epsilon * dataset_size;

  for (uint32_t i = 0; i < n_values; i++) {
    uint32_t val = ground_truth[i].val;
    uint32_t count = ground_truth[i].count;
    uint32_t estimate = cms_point_query_int(cms, val);
    if (estimate < count) {
      printf("Implementation error: cms estimate cannot be lower than the true count");
      return 1;
    }
    uint64_t abs_error = (estimate > count) ? (estimate - count) : 0;
    total_abs_error += abs_error;
    if (abs_error > max_abs_error)
      max_abs_error = abs_error;
    if (estimate == count)
      total_exact_matches++;
    if (abs_error <= error_bound)
      total_within_bound++;
  }

  printf("\nAccuracy Test Summary\n");
  printf("Avg absolute error: %.2f\n", (double)total_abs_error / n_values);
  printf("Max absolute error: %" PRIu64 "\n", max_abs_error);
  printf("Exact matches: %" PRIu64 " over %u items (%.2f%%)\n", total_exact_matches, n_values, (double)(total_exact_matches / n_values) * 100);
  printf("Within error bound: %" PRIu64 " over %u items (%.2f%%)\n\n", total_within_bound, n_values, (double)(total_within_bound / n_values) * 100);
  return 0;
}

// function to test the inner product between two CMS
// this function will be refactored once the code will be properly tested
void test_inner_product_demo() {
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

void test_basic_update_query_demo() {
  printf("Start Test: Basic Update and Query \n");
  uint32_t A_sum = 0;
  uint32_t B_sum = 0;

  CountMinSketch cms;
  cms_init(&cms, 0.1, 0.1, 2147483647);

  uint32_t item_a = 123;
  cms_update_int(&cms, item_a, 10);
  A_sum += 10;

  uint32_t item_b = 456;
  cms_update_int(&cms, item_b, 5);
  B_sum += 5;

  uint32_t stima_a = cms_point_query_int(&cms, item_a);
  uint32_t stima_b = cms_point_query_int(&cms, item_b);
  uint32_t stima_c = cms_point_query_int(&cms, 999);

  printf("Estimation for A (123): %u (expected: >= %u)\n", stima_a, A_sum);
  printf("Estimation for B (456): %u (expected: >= %u)\n", stima_b, B_sum);
  printf("Estimation for C (999): %u (expected: 0 or a small number)\n", stima_c);

  cms_free(&cms);
}

void test_range_query_demo() {
  printf("Start Test: Range Query\n");
  CountMinSketch cms;

  cms_init(&cms, 0.1, 0.1, 2147483647);

  int test_start = 100;
  int test_end = 110;
  uint32_t true_sum = 0;

  cms_update_int(&cms, 100, 5);
  true_sum += 5;

  cms_update_int(&cms, 105, 3);
  true_sum += 3;

  cms_update_int(&cms, 110, 2);
  true_sum += 2;

  cms_update_int(&cms, 50, 10);
  cms_update_int(&cms, 200, 8);

  uint32_t range_stima = cms_range_query_int(&cms, test_start, test_end);

  printf("Range 100-110: CMS Estimate: %u (expected: >= %u)\n", range_stima, true_sum);

  cms_free(&cms);
}

void test_basic_update_query(CountMinSketch* cms, uint32_t true_A, uint32_t true_B) {
  printf("Start Test: Basic Update and Query\n");

  uint32_t stima_A = cms_point_query_int(cms, 123);
  uint32_t stima_B = cms_point_query_int(cms, 456);

  printf("Item 123 → estimation: %u, real: %u\n", stima_A, true_A);
  printf("Item 456 → estimation: %u, real: %u\n", stima_B, true_B);

  uint32_t stima_C = cms_point_query_int(cms, 999);
  printf("Item 999 → estimation: %u (expected: 0 or a small number)\n", stima_C);
}

void test_range_query(CountMinSketch* cms, uint32_t true_range_sum) {
  printf("Start Test: Range Query\n");

  uint32_t stima = cms_range_query_int(cms, 100, 110);

  printf("Range 100–110 → estimation: %u, real: %u\n", stima, true_range_sum);
}

void test_inner_product(CountMinSketch* cms_a, CountMinSketch* cms_b) {
  printf("Start Test: Inner Product\n");

  uint32_t result = cms_inner_product(cms_a, cms_b);

  printf("Inner product = %u\n", result);
}

// Count lines in ground truth file
uint32_t count_lines(const char* filename) {
  FILE* fp = fopen(filename, "r");
  if (!fp)
    return 0;
  
  uint32_t count = 0;
  char line[100];
  while (fgets(line, sizeof(line), fp)) {
    count++;
  }
  fclose(fp);
  return count;
}

/*int main(int argc, char* argv[]) {
  test_basic_update_query_demo();
  test_range_query_demo();
  test_inner_product_demo();

  return 0;
}
  */

