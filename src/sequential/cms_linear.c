#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../core/count_min_sketch.h"

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

  uint32_t true_A_sum = 0, true_B_sum = 0, true_Range_sum = 0;

  FILE* fp = fopen(FILENAME, "r");
  if (!fp) {
    fprintf(stderr, "Rank 0: cannot open file %s\n", FILENAME);
    return 2;
  }

  // Update CMS while reading file
  char line[64];
  while (fgets(line, sizeof(line), fp)) {
    uint32_t v = (uint32_t)atoi(line);
    cms_update_int(&cms, v, 1);

    if (v == 123) true_A_sum++;
    if (v == 456) true_B_sum++;
    if (v >= 100 && v <= 110) true_Range_sum++;
  }
  fclose(fp);

  // Point Query Test
  double t_point_start = MPI_Wtime();
  test_basic_update_query(&cms, true_A_sum, true_B_sum);
  double t_point_end = MPI_Wtime();

  // Range Query Test
  double t_range_start = MPI_Wtime();
  test_range_query(&cms, true_Range_sum);
  double t_range_end = MPI_Wtime();

  // Inner Product Test
  double t_inner_start = MPI_Wtime();
  uint64_t inner_prod = cms_inner_product(&cms, &cms);
  double t_inner_end = MPI_Wtime();

  printf("\nInner Product Test\n");
  printf("Inner product (self): %lu\n", (unsigned long)inner_prod);

  printf("\nQuery Timing:\n");
  printf("Point query time:  %f s\n", t_point_end - t_point_start);
  printf("Range query time:  %f s\n", t_range_end - t_range_start);
  printf("Inner product time: %f s\n", t_inner_end - t_inner_start);

  cms_free(&cms);

  double t_end = MPI_Wtime();
  printf("Total time: %f seconds\n", t_point_start - t_start);
  printf("Total execution time: %.2f seconds\n", t_end - t_start);

  MPI_Finalize();
  return 0;
}