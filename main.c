#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "count_min_sketch.h"


int main(int argc, char* argv[]) {
    int comm_sz, my_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    double t_start = MPI_Wtime();
    srand(time(NULL) + my_rank);

    CountMinSketch local_cms;
    if (cms_init(&local_cms, EPSILON, DELTA, PRIME) != 0) {
        if (my_rank == 0) fprintf(stderr, "Error in cms_init\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Bcast(local_cms.hashFunctions,
              local_cms.depth * sizeof(UniversalHash),
              MPI_BYTE, 0, MPI_COMM_WORLD);

    const char* FILENAME = argv[1];

    uint32_t* all_items = NULL;
    uint64_t total_items = 0;
    uint32_t true_A_sum = 0, true_B_sum = 0, true_Range_sum = 0;

    /* Rank 0 reads the entire file */
    if (my_rank == 0) {
        printf("Parallel Count-Min Sketch (MAINV1)\n");

        FILE* fp = fopen(FILENAME, "r");
        if (!fp) {
            fprintf(stderr, "Rank 0: cannot open file %s\n", FILENAME);
            MPI_Abort(MPI_COMM_WORLD, 2);
        }

        char line[64];
        while (fgets(line, sizeof(line), fp))
            total_items++;
        rewind(fp);

        all_items = malloc(total_items * sizeof(uint32_t));
        if (!all_items) {
            fprintf(stderr, "Rank 0: malloc failed\n");
            MPI_Abort(MPI_COMM_WORLD, 3);
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

        /*  DATASET INFO: file size su disco */
        FILE* fp_check = fopen(FILENAME, "rb");
        if (!fp_check) {
            fprintf(stderr, "Rank 0: cannot open file %s to check size\n", FILENAME);
            MPI_Abort(MPI_COMM_WORLD, 4);
        }
        fseek(fp_check, 0, SEEK_END);
        long fsize = ftell(fp_check);
        fclose(fp_check);

        double file_size_mb = (double)fsize / (1024.0 * 1024.0);
        printf("\n DATASET INFO \n");
        printf("Dataset file: %s\n", FILENAME);
        printf("File size on disk: %.2f MB\n", file_size_mb);
        printf("\n");
    }

    MPI_Bcast(&total_items, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);

    int* send_counts = malloc(comm_sz * sizeof(int));
    int* displs = malloc(comm_sz * sizeof(int));
    int base = total_items / comm_sz;
    int remainder = total_items % comm_sz;

    for (int i = 0; i < comm_sz; i++) {
        send_counts[i] = base + (i < remainder ? 1 : 0);
        displs[i] = (i == 0) ? 0 : displs[i - 1] + send_counts[i - 1];
    }

    uint32_t* local_items = malloc(send_counts[my_rank] * sizeof(uint32_t));
    if (!local_items) {
        fprintf(stderr, "Rank %d: malloc failed\n", my_rank);
        MPI_Abort(MPI_COMM_WORLD, 4);
    }

    MPI_Scatterv(all_items, send_counts, displs, MPI_UINT32_T,
                 local_items, send_counts[my_rank], MPI_UINT32_T,
                 0, MPI_COMM_WORLD);

    if (my_rank == 0) {
        free(all_items);
        all_items = NULL;
    }

    for (int i = 0; i < send_counts[my_rank]; i++)
        cms_update_int(&local_cms, local_items[i], 1);

    CountMinSketch global_cms;
    if (my_rank == 0) {
        if (cms_init(&global_cms, EPSILON, DELTA, PRIME) != 0) {
            fprintf(stderr, "Error initializing global_cms\n");
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

    if (my_rank == 0) {
        double t_point_start = MPI_Wtime();
        test_basic_update_query(&global_cms, true_A_sum, true_B_sum);
        double t_point_end = MPI_Wtime();

        double t_range_start = MPI_Wtime();
        test_range_query(&global_cms, true_Range_sum);
        double t_range_end = MPI_Wtime();

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
    free(local_items);
    free(send_counts);
    free(displs);

    double t_end = MPI_Wtime();
    if (my_rank == 0) {
        printf("Total time: %f seconds\n", t_end - t_start);
        printf("\n --------------------------------------\n");
    }

    MPI_Finalize();
    return 0;
}
