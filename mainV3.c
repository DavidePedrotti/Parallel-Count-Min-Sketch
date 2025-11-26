#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "count_min_sketch.h"

#define MAX_LINE_LEN 64

/*
 * MPI-only version with sorted dataset
 * node 0 reads the dataset to count the lines, then each node reads its corresponding data
 */

int main(int argc, char* argv[]) {
  int comm_sz, my_rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  // Start TIMER
  double t_start = MPI_Wtime();
  srand(time(NULL) + my_rank);

  const char* FILENAME = argv[1];  // file already sorted
  size_t total_items = 0;

  CountMinSketch local_cms;
  if (cms_init(&local_cms, EPSILON, DELTA, PRIME) != 0) {
    if (my_rank == 0) fprintf(stderr, "Error initializing local CMS\n");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  // Rank 0 reads the total number of lines
  if (my_rank == 0) {
    FILE* fp = fopen(FILENAME, "r");
    if (!fp) {
      fprintf(stderr, "Cannot open file %s\n", FILENAME);
      MPI_Abort(MPI_COMM_WORLD, 2);
    }
    char line[MAX_LINE_LEN];
    while (fgets(line, MAX_LINE_LEN, fp)) total_items++;
    fclose(fp);
  }

  // Broadcast the total number of items to all ranks
  MPI_Bcast(&total_items, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);

  // Calculate the interval for each rank
  size_t chunk_size = total_items / comm_sz;
  size_t start_idx = my_rank * chunk_size;
  size_t end_idx = (my_rank == comm_sz - 1) ? total_items : start_idx + chunk_size;
  size_t local_count = end_idx - start_idx;

  uint32_t* local_items = malloc(local_count * sizeof(uint32_t));
  if (!local_items) {
    fprintf(stderr, "Rank %d: malloc failed\n", my_rank);
    MPI_Abort(MPI_COMM_WORLD, 3);
  }

  // Each rank sequentially reads its portion
  FILE* fp = fopen(FILENAME, "r");
  if (!fp) {
    fprintf(stderr, "Rank %d: cannot open file %s\n", my_rank, FILENAME);
    MPI_Abort(MPI_COMM_WORLD, 4);
  }
  char line[MAX_LINE_LEN];
  size_t idx = 0;
  size_t curr_line = 0;
  while (fgets(line, MAX_LINE_LEN, fp)) {
    if (curr_line >= start_idx && curr_line < end_idx) {
      local_items[idx++] = (uint32_t)atoi(line);
    }
    curr_line++;
    if (curr_line >= end_idx) break;  // stop when the chunk is read
  }
  fclose(fp);

  for (size_t i = 0; i < local_count; i++) {
    cms_update_int(&local_cms, local_items[i], 1);
  }

  CountMinSketch global_cms;
  if (my_rank == 0) {
    if (cms_init(&global_cms, EPSILON, DELTA, PRIME) != 0) {
      fprintf(stderr, "Error initializing global CMS\n");
      MPI_Abort(MPI_COMM_WORLD, 5);
    }
    global_cms.total = 0;
    for (uint32_t i = 0; i < global_cms.depth; i++)
      global_cms.hashFunctions[i] = local_cms.hashFunctions[i];
  }

  for (uint32_t d = 0; d < local_cms.depth; d++) {
    MPI_Reduce(local_cms.table[d],
               (my_rank == 0 ? global_cms.table[d] : NULL),
               local_cms.width, MPI_UINT32_T,
               MPI_SUM, 0, MPI_COMM_WORLD);
  }

  MPI_Reduce(&local_cms.total,
             (my_rank == 0 ? &global_cms.total : NULL),
             1, MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);

  // Run tests on rank 0
  if (my_rank == 0) {
    uint32_t true_123 = 0, true_456 = 0, true_range = 0;

    FILE* fp_test = fopen(FILENAME, "r");
    if (fp_test) {
      while (fgets(line, MAX_LINE_LEN, fp_test)) {
        uint32_t val = (uint32_t)atoi(line);
        if (val == 123) true_123++;
        if (val == 456) true_456++;
        if (val >= 100 && val <= 110) true_range++;
      }
      fclose(fp_test);
    }

    test_basic_update_query(&global_cms, true_123, true_456);
    test_range_query(&global_cms, true_range);

    cms_free(&global_cms);
  }

  double t_end = MPI_Wtime();
  if (my_rank == 0) {
    printf("Total time V3: %f seconds\n", t_end - t_start);
  }

  cms_free(&local_cms);
  free(local_items);
  local_items = NULL;

  MPI_Finalize();
  return 0;
}
