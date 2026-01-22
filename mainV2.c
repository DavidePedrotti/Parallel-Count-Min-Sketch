#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "count_min_sketch.h"

#define MAX_LINE_LEN 64

/*
 * MPI-only version using MPI_File
 * Parallel Count-Min Sketch
 */

int main(int argc, char* argv[]) {
  int comm_sz, my_rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  //   TOTAL TIME START
  MPI_Barrier(MPI_COMM_WORLD);
  double t_start = MPI_Wtime();

  srand(time(NULL) + my_rank);

  if (my_rank == 0)
    printf("Parallel Count-Min Sketch with MPI-I/O (full chunk reading)\n");

  // CMS INITIALIZATION
  CountMinSketch local_cms;
  if (cms_init(&local_cms, EPSILON, DELTA, PRIME) != 0) {
    if (my_rank == 0)
      fprintf(stderr, "Error in cms_init\n");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  // Broadcast hash functions so all CMS are consistent
  MPI_Bcast(local_cms.hashFunctions,
            local_cms.depth * sizeof(UniversalHash),
            MPI_BYTE,
            0,
            MPI_COMM_WORLD);

  const char* FILENAME = argv[1];

  // MPI-I/O + PARSING
  MPI_Barrier(MPI_COMM_WORLD);
  double t_io_start = MPI_Wtime();

  MPI_File fh;
  MPI_File_open(MPI_COMM_WORLD, FILENAME,
                MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);

  MPI_Offset file_size;
  MPI_File_get_size(fh, &file_size);
  file_size--;  

  MPI_Offset approx_chunk = file_size / comm_sz;
  MPI_Offset my_start = my_rank * approx_chunk;
  MPI_Offset my_end =
      (my_rank == comm_sz - 1) ? file_size : my_start + approx_chunk;

  // Align chunk start to newline 
  if (my_rank != 0) {
    char c;
    MPI_File_read_at(fh, my_start - 1,
                     &c, 1, MPI_CHAR, MPI_STATUS_IGNORE);
    while (c != '\n' && my_start < my_end) {
      my_start++;
      MPI_File_read_at(fh, my_start - 1,
                       &c, 1, MPI_CHAR, MPI_STATUS_IGNORE);
    }
  }

  MPI_Offset my_chunk_size = my_end - my_start + 1;
  char* buffer = malloc(my_chunk_size + 1);
  if (!buffer) MPI_Abort(MPI_COMM_WORLD, 99);

  MPI_File_read_at_all(fh, my_start,
                       buffer, my_chunk_size,
                       MPI_CHAR, MPI_STATUS_IGNORE);
  buffer[my_chunk_size] = '\0';

    // Trim incomplete line at the end (except last rank)
  if (my_rank != comm_sz - 1) {
    char* last_nl = strrchr(buffer, '\n');
    if (last_nl)
      *(last_nl + 1) = '\0';
    else
      buffer[0] = '\0';
  }

  MPI_File_close(&fh);

    // Count lines
  int line_count = 0;
  for (char* p = buffer; *p; p++)
    if (*p == '\n') line_count++;

  uint32_t* local_items =
      malloc(line_count * sizeof(uint32_t));
  if (!local_items) MPI_Abort(MPI_COMM_WORLD, 99);

  int idx = 0;
  char* token = strtok(buffer, "\n");
  while (token) {
    local_items[idx++] = (uint32_t)atoi(token);
    token = strtok(NULL, "\n");
  }
  free(buffer);

  MPI_Barrier(MPI_COMM_WORLD);
  double t_io_end = MPI_Wtime();

  // CMS UPDATE and calculate local ground truth
  MPI_Barrier(MPI_COMM_WORLD);
  double t_update_start = MPI_Wtime();

  uint32_t local_123 = 0, local_456 = 0, local_range = 0;
  for (int i = 0; i < idx; i++) {
    uint32_t val = local_items[i];
    cms_update_int(&local_cms, val, 1);
    
    // Count ground truth values
    if (val == 123) local_123++;
    if (val == 456) local_456++;
    if (val >= 100 && val <= 110) local_range++;
  }

  free(local_items);

  MPI_Barrier(MPI_COMM_WORLD);
  double t_update_end = MPI_Wtime();

  /* ======================
     REDUCTION
     ====================== */
  MPI_Barrier(MPI_COMM_WORLD);
  double t_reduce_start = MPI_Wtime();

  CountMinSketch global_cms;
  if (my_rank == 0) {
    cms_init(&global_cms, EPSILON, DELTA, PRIME);
    global_cms.total = 0;
    for (uint32_t i = 0; i < global_cms.depth; i++)
      global_cms.hashFunctions[i] = local_cms.hashFunctions[i];
  }

  for (uint32_t d = 0; d < local_cms.depth; d++) {
    MPI_Reduce(local_cms.table[d],
               (my_rank == 0 ? global_cms.table[d] : NULL),
               local_cms.width,
               MPI_UINT32_T,
               MPI_SUM,
               0,
               MPI_COMM_WORLD);
  }

  MPI_Reduce(&local_cms.total,
             (my_rank == 0 ? &global_cms.total : NULL),
             1, MPI_UINT32_T, MPI_SUM,
             0, MPI_COMM_WORLD);

  // Reduce ground truth counts
  uint32_t true_123 = 0, true_456 = 0, true_range = 0;
  MPI_Reduce(&local_123, &true_123, 1, MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&local_456, &true_456, 1, MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&local_range, &true_range, 1, MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);

  MPI_Barrier(MPI_COMM_WORLD);
  double t_reduce_end = MPI_Wtime();

  // TESTS (rank 0)
  if (my_rank == 0) {
    // Point Query Test
    double t_point_start = MPI_Wtime();
    test_basic_update_query(&global_cms, true_123, true_456);
    double t_point_end = MPI_Wtime();
    
    // Range Query Test
    double t_range_start = MPI_Wtime();
    test_range_query(&global_cms, true_range);
    double t_range_end = MPI_Wtime();
    
    // Inner Product Test
    double t_inner_start = MPI_Wtime();
    uint64_t inner_prod = cms_inner_product(&global_cms, &global_cms);
    double t_inner_end = MPI_Wtime();
    printf("\nInner Product Test\n");
    printf("Inner product (self): %lu\n", (unsigned long)inner_prod);
    
    printf("\nQuery Timing:\n");
    printf("Point query time:  %f s\n", t_point_end - t_point_start);
    printf("Range query time:  %f s\n", t_range_end - t_range_start);
    printf("Inner product time: %f s\n", t_inner_end - t_inner_start);
    
    cms_free(&global_cms);
  }

  cms_free(&local_cms);

    // TOTAL TIME END
  MPI_Barrier(MPI_COMM_WORLD);
  double t_end = MPI_Wtime();

  if (my_rank == 0) {
    printf("\nTiming summary:\n");
    printf("I/O + parsing time: %f s\n", t_io_end - t_io_start);
    printf("CMS update time:   %f s\n", t_update_end - t_update_start);
    printf("Reduction time:    %f s\n", t_reduce_end - t_reduce_start);
    printf("Total time:        %f s\n", t_end - t_start);
  }

  MPI_Finalize();
  return 0;
}

