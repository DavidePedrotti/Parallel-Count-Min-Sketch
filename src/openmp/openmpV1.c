#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../core/count_min_sketch_hybridV1.h"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
    return 1;
  }

  double t_start, t_io_start, t_io_end, t_update_start, t_update_end, t_reduce_end, t_end;
  t_start = omp_get_wtime();

  srand(time(NULL));

  printf("Parallel Count-Min Sketch (OpenMP only, per-thread private CMS)\n");

  // CMS initialization
  CountMinSketch global_cms;
  cms_init(&global_cms, EPSILON, DELTA, PRIME);

  // MEMORY USAGE
  size_t cms_table_bytes = global_cms.depth * global_cms.width * sizeof(uint32_t);
  size_t cms_hash_bytes = global_cms.depth * sizeof(UniversalHash);
  size_t cms_bytes = cms_table_bytes + cms_hash_bytes;

  int omp_threads = omp_get_max_threads();
  size_t cms_threads_bytes = omp_threads * cms_bytes;
  size_t cms_total_rank_bytes = cms_bytes + cms_threads_bytes;

  printf("\n MEMORY USAGE \n");
  printf("CMS total global: %.2f MB\n", cms_total_rank_bytes / (1024.0 * 1024.0));

  // Read entire file (serial)
  t_io_start = omp_get_wtime();
  FILE* f = fopen(argv[1], "r");
  if (!f) {
    perror("fopen");
    return 1;
  }

  size_t cap = 1 << 20;
  size_t n = 0;
  uint32_t* items = malloc(cap * sizeof(uint32_t));

  while (!feof(f)) {
    if (n == cap) {
      cap *= 2;
      items = realloc(items, cap * sizeof(uint32_t));
    }
    fscanf(f, "%u", &items[n++]);
  }
  fclose(f);

  double dataset_size_mb = (double)(n * sizeof(uint32_t)) / (1024.0 * 1024.0);

  printf("\n DATASET INFO \n");
  printf("Dataset file: %s\n", argv[1]);
  printf("Dataset size: %.2f MB\n", dataset_size_mb);

  t_io_end = omp_get_wtime();

  // OpenMP parallel update
  t_update_start = omp_get_wtime();

  uint32_t local_123 = 0, local_456 = 0, local_range = 0;

#pragma omp parallel
  {
    CountMinSketch thread_cms;
    cms_init_private(&thread_cms, &global_cms);

    uint32_t local_123_private = 0;
    uint32_t local_456_private = 0;
    uint32_t local_range_private = 0;

#pragma omp for
    for (size_t i = 0; i < n; i++) {
      uint32_t val = items[i];
      cms_update_int_parallel(&thread_cms, val, 1);

      if (val == 123) local_123_private++;
      if (val == 456) local_456_private++;
      if (val >= 100 && val <= 110) local_range_private++;
    }

#pragma omp critical
    {
      for (uint32_t d = 0; d < global_cms.depth; d++)
        for (uint32_t w = 0; w < global_cms.width; w++)
          global_cms.table[d][w] += thread_cms.table[d][w];

      global_cms.total += thread_cms.total;
      local_123 += local_123_private;
      local_456 += local_456_private;
      local_range += local_range_private;
    }

    cms_free_private(&thread_cms);
  }

  t_update_end = omp_get_wtime();

  t_reduce_end = t_update_end;

  printf("\n ITEM ESTIMATIONS \n");
  printf("Item 123 → estimation: %u, real: %u\n",
         cms_point_query_int(&global_cms, 123), local_123);
  printf("Item 456 → estimation: %u, real: %u\n",
         cms_point_query_int(&global_cms, 456), local_456);
  printf("Item 999 → estimation: %u (expected: 0 or small)\n",
         cms_point_query_int(&global_cms, 999));
  printf("Range 100–110 → estimation: %u, real: %u\n",
         cms_range_query_int(&global_cms, 100, 110), local_range);

  t_end = omp_get_wtime();

  printf("\n TIMINGS \n");
  printf("Total time: %f seconds\n", t_end - t_start);
  printf("I/O + parsing time: %f s\n", t_io_end - t_io_start);
  printf("CMS update time: %f s\n", t_update_end - t_update_start);
  printf("\n --------------------------------------\n");

  free(items);
  cms_free(&global_cms);

  return 0;
}
