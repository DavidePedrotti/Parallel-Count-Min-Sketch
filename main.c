#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "count_min_sketch.h"


int main(int argc, char* argv[]) {
  int comm_sz, my_rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  srand(time(NULL) + my_rank);

  CountMinSketch local_cms;
  if (cms_init(&local_cms, EPSILON, DELTA, PRIME) != 0) {
      if (my_rank == 0) fprintf(stderr, "Error in cms_init\n");
      MPI_Abort(MPI_COMM_WORLD, 1);
  }

  local_cms.total = 0;

  if (my_rank == 0) {
        universal_hash_array_init(local_cms.hashFunctions, PRIME, local_cms.width, local_cms.depth);
  }

  MPI_Bcast(local_cms.hashFunctions, local_cms.depth * sizeof(UniversalHash), MPI_BYTE, 0, MPI_COMM_WORLD);

  const int N_PER_PROC = 1000;
  const int N_GLOBAL_ITEMS = N_PER_PROC * comm_sz;
  const char* FILENAME = "dataset.txt";

  uint32_t* all_items = NULL;
  uint32_t true_A_sum = 0, true_B_sum = 0, true_Range_sum = 0;

  if (my_rank == 0) {
    
    FILE *fp = fopen(FILENAME, "r");
    if (fp == NULL) {
        fprintf(stderr, "Rank 0: ERROR: Impossible to open the file %s\n", FILENAME);
        MPI_Abort(MPI_COMM_WORLD, 2);
    }

    all_items = malloc(N_GLOBAL_ITEMS * sizeof(uint32_t));
    if (all_items == NULL) {
        fprintf(stderr, "Rank 0: ERROR: malloc failure for all_items\n");
        fclose(fp);
        MPI_Abort(MPI_COMM_WORLD, 2);
    }

    for (int i = 0; i < N_GLOBAL_ITEMS; i++) {
        if (fscanf(fp, "%u", &all_items[i]) != 1) {
            fprintf(stderr, "Rank 0: ERROR: File %s too short", FILENAME);
            fclose(fp);
            free(all_items);
            MPI_Abort(MPI_COMM_WORLD, 3);
        }

        uint32_t item = all_items[i];
        if (item == 123) { 
            true_A_sum++; 
        } else if (item == 456) { 
            true_B_sum++; 
        } else if (item >= 100 && item <= 110) { 
            true_Range_sum++;
        }
    }

    fclose(fp);
  }

  uint32_t* local_items = malloc(N_PER_PROC * sizeof(uint32_t));
  if (local_items == NULL) {
      fprintf(stderr, "Rank %d: ERROR: malloc failure for local_items\n", my_rank);
      MPI_Abort(MPI_COMM_WORLD, 4);
  }

  MPI_Scatter(all_items, N_PER_PROC, MPI_UINT32_T, local_items, N_PER_PROC, MPI_UINT32_T, 0, MPI_COMM_WORLD);

  if (my_rank == 0) {
      free(all_items);
      all_items = NULL;
  }

  for (int i = 0; i < N_PER_PROC; i++) {
    cms_update_int(&local_cms, local_items[i], 1);
  }

  CountMinSketch global_cms;
  uint32_t* global_table_buffer = NULL; 

  if (my_rank == 0) {
      if (cms_init(&global_cms, EPSILON, DELTA, PRIME) != 0) {
          fprintf(stderr, "Error initializing global_cms\n");
          MPI_Abort(MPI_COMM_WORLD, 1);
      }
      global_cms.total = 0; 
      
      for (uint32_t i = 0; i < global_cms.depth; i++) {
          global_cms.hashFunctions[i] = local_cms.hashFunctions[i];
      }
  }

  for (uint32_t j = 0; j < local_cms.depth; j++) {
      if (my_rank == 0) {
          global_table_buffer = global_cms.table[j];
      }

      MPI_Reduce(local_cms.table[j], global_table_buffer, local_cms.width, MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);
  }

  MPI_Reduce(&local_cms.total, &global_cms.total, 1, MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);
  
  if (my_rank == 0) {
      test_basic_update_query(&global_cms, true_A_sum, true_B_sum);
      test_range_query(&global_cms, true_Range_sum);

      CountMinSketch cms_b;
      cms_init(&cms_b, EPSILON, DELTA, PRIME);
      cms_b.total = 0;
      
      for (uint32_t i = 0; i < cms_b.depth; i++) {
          cms_b.hashFunctions[i] = global_cms.hashFunctions[i];
      }
      
      cms_update_int(&cms_b, 123, 5);
      cms_update_int(&cms_b, 456, 3);
      cms_update_int(&cms_b, 789, 2);
      
      test_inner_product(&global_cms, &cms_b);
      cms_free(&cms_b);


      cms_free(&global_cms);
  }

  cms_free(&local_cms);

  free(local_items);
  local_items = NULL;

  MPI_Finalize();
  return 0;
}

