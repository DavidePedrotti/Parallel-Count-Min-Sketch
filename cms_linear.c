#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "count_min_sketch.h"

typedef struct {
  uint32_t val;
  uint32_t count;
} RealCount;

/*
 * Linear CMS version
 */

// tests cms accuracy vs the ground truth
void test_cms_accuracy(CountMinSketch* cms, RealCount* ground_truth, uint32_t n_values, uint32_t dataset_size) {
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
      return;
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
  printf("Max absolute error: %lu\n", max_abs_error);
  printf("Exact matches: %lu over %u items (%.2f%%)\n", total_exact_matches, n_values, (double)(total_exact_matches / n_values) * 100);
  printf("Within error bound: %lu over %u items (%.2f%%)\n\n", total_within_bound, n_values, (double)(total_within_bound / n_values) * 100);
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

int main(int argc, char* argv[]) {
  MPI_Init(&argc, &argv);

  double t_start = MPI_Wtime();
  srand(time(NULL));

  CountMinSketch cms;
  if (cms_init(&cms, EPSILON, DELTA, PRIME) != 0) {
    fprintf(stderr, "Error in cms_init\n");
    return 1;
  }

  const char* FILENAME = argv[1];
  const char* FOLDER = argv[2];

  uint32_t* all_items = NULL;
  uint64_t total_items = 0;
  uint32_t true_A_sum = 0, true_B_sum = 0, true_Range_sum = 0;

  FILE* fp = fopen(FILENAME, "r");
  if (!fp) {
    fprintf(stderr, "Rank 0: cannot open file %s\n", FILENAME);
    return 2;
  }

  // Conta righe
  char line[64];
  while (fgets(line, sizeof(line), fp)) total_items++;
  rewind(fp);

  all_items = malloc(total_items * sizeof(uint32_t));
  if (!all_items) {
    fprintf(stderr, "Rank 0: malloc failed\n");
    return 3;
  }

  uint64_t idx = 0;
  while (fgets(line, sizeof(line), fp)) {
    uint32_t v = (uint32_t)atoi(line);
    all_items[idx++] = v;

    if (v == 123) true_A_sum++;
    if (v == 456) true_B_sum++;
    if (v >= 100 && v <= 110) true_Range_sum++;
  }
  fclose(fp);

  // Aggiorno CMS locale
  for (int i = 0; i < total_items; i++) {
    cms_update_int(&cms, all_items[i], 1);
  }

  double t_before_accuracy = MPI_Wtime();

  const char* base_filename = strrchr(FILENAME, '/');
  base_filename = base_filename ? base_filename + 1 : FILENAME;  // base_filename+1 moves past '/'
  char total_count_filename[100];
  snprintf(total_count_filename, sizeof(total_count_filename), "%s/total_%s", FOLDER, base_filename);
  RealCount* count = load_count(total_count_filename, 10000);  // TODO: change this so that it's not hardcoded
  if (!count) {
    fprintf(stderr, "Error: cannot load the ground truth file %s\n", total_count_filename);
    cms_free(&cms);
    free(all_items);
    MPI_Finalize();
    return 4;
  }

  test_cms_accuracy(&cms, count, 10000, 50000);
  double t_after_accuracy = MPI_Wtime();

  test_basic_update_query(&cms, true_A_sum, true_B_sum);
  test_range_query(&cms, true_Range_sum);
  cms_free(&cms);

  free(all_items);

  double t_end = MPI_Wtime();
  printf("\nTotal time taken to update CMS: %.2f seconds\n", t_before_accuracy - t_start);
  printf("Total time taken for accuracy test: %.2f seconds\n", t_after_accuracy - t_before_accuracy);
  printf("Total execution time (including queries): %.2f seconds\n", t_end - t_start);

  MPI_Finalize();
  return 0;
}