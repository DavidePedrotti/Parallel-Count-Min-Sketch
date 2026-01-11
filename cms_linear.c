#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "count_min_sketch.h"


/*
 * Linear CMS version
 */

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

  char* total_count_filename[100];
  strcpy(total_count_filename, "total_");
  strcat(total_count_filename, FILENAME);
  RealCount* count = load_count(total_count_filename, 10000);  // TODO: change this so that it's not hardcoded
  test_cms_accuracy(&cms, &count, 10000, 50000);

  // test_basic_update_query(&cms, true_A_sum, true_B_sum);
  // test_range_query(&cms, true_Range_sum);
  cms_free(&cms);

  free(all_items);

  double t_end = MPI_Wtime();
  printf("Total time (all elements, V1 structure) = %f seconds\n", t_end - t_start);

  MPI_Finalize();
  return 0;
}