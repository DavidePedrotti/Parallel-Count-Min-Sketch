#include <mpi.h>
#include <omp.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "count_min_sketch_hybridV1.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    int comm_sz, my_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    // Timing variables
    double t_start, t_io_start, t_io_end;
    double t_update_start, t_update_end;
    double t_reduce_start, t_reduce_end;
    double t_end;

    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();

    srand(time(NULL) + my_rank);

    if (my_rank == 0)
        printf("Parallel Count-Min Sketch (V1: Hybrid MPI + OpenMP, per-thread private CMS)\n");

    // CMS initialization 
    CountMinSketch local_cms;
    if (cms_init(&local_cms, EPSILON, DELTA, PRIME) != 0) {
        if (my_rank == 0) fprintf(stderr, "Error initializing CMS\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Bcast(local_cms.hashFunctions,
              local_cms.depth * sizeof(UniversalHash),
              MPI_BYTE, 0, MPI_COMM_WORLD);

    size_t cms_table_bytes =
        local_cms.depth * local_cms.width * sizeof(uint32_t);
    size_t cms_hash_bytes =
        local_cms.depth * sizeof(UniversalHash);
    size_t cms_bytes = cms_table_bytes + cms_hash_bytes;

    int omp_threads = omp_get_max_threads();
    size_t cms_threads_bytes = omp_threads * cms_bytes;
    size_t cms_total_rank_bytes = cms_bytes + cms_threads_bytes;
    size_t cms_total_global_bytes = cms_total_rank_bytes * comm_sz;

    if (my_rank == 0) {
        printf("\n MEMORY USAGE \n");
        printf("CMS total global: %.2f MB\n",
               cms_total_global_bytes / (1024.0 * 1024.0));
    }

    const char* FILENAME = argv[1];

    // MPI I/O 
    MPI_Barrier(MPI_COMM_WORLD);
    t_io_start = MPI_Wtime();

    MPI_File fh;
    MPI_File_open(MPI_COMM_WORLD, FILENAME, MPI_MODE_RDONLY,
                  MPI_INFO_NULL, &fh);

    MPI_Offset file_size;
    MPI_File_get_size(fh, &file_size);

    double dataset_size_mb = file_size / (1024.0 * 1024.0);
    if (my_rank == 0) {
        printf("\n DATASET INFO \n");
        printf("Dataset file: %s\n", FILENAME);
        printf("Dataset size: %.2f MB\n", dataset_size_mb);
    }

    MPI_Offset approx_chunk = file_size / comm_sz;
    MPI_Offset my_start = my_rank * approx_chunk;
    MPI_Offset my_end =
        (my_rank == comm_sz - 1)
            ? (file_size - 1)
            : (my_start + approx_chunk - 1);

    if (my_rank != 0) {
        char c;
        MPI_File_read_at(fh, my_start - 1, &c, 1,
                         MPI_CHAR, MPI_STATUS_IGNORE);
        while (c != '\n' && my_start < my_end) {
            my_start++;
            MPI_File_read_at(fh, my_start - 1, &c, 1,
                             MPI_CHAR, MPI_STATUS_IGNORE);
        }
    }

    MPI_Offset my_chunk_size = my_end - my_start + 1;
    char* buffer = malloc((size_t)my_chunk_size + 1);
    if (!buffer) MPI_Abort(MPI_COMM_WORLD, 99);

    MPI_Offset remaining = my_chunk_size;
    MPI_Offset offset = my_start;
    char* ptr = buffer;

    while (remaining > 0) {
        int to_read = remaining > INT_MAX ? INT_MAX : (int)remaining;
        MPI_File_read_at(fh, offset, ptr, to_read,
                         MPI_CHAR, MPI_STATUS_IGNORE);
        remaining -= to_read;
        offset += to_read;
        ptr += to_read;
    }
    buffer[my_chunk_size] = '\0';

    if (my_rank != comm_sz - 1) {
        char* last_nl = strrchr(buffer, '\n');
        if (last_nl)
            *(last_nl + 1) = '\0';
        else
            buffer[0] = '\0';
    }

    MPI_File_close(&fh);

    // Count lines
    size_t line_count = 0;
    for (char* p = buffer; *p; p++)
        if (*p == '\n') line_count++;

    uint32_t* local_items = malloc(line_count * sizeof(uint32_t));
    if (!local_items) MPI_Abort(MPI_COMM_WORLD, 99);

    size_t idx = 0;
    char* token = strtok(buffer, "\n");
    while (token) {
        local_items[idx++] = (uint32_t)strtoul(token, NULL, 10);
        token = strtok(NULL, "\n");
    }
    free(buffer);

    MPI_Barrier(MPI_COMM_WORLD);
    t_io_end = MPI_Wtime();

    // CMS update + local counts 
    MPI_Barrier(MPI_COMM_WORLD);
    t_update_start = MPI_Wtime();

    uint32_t local_123 = 0, local_456 = 0, local_range = 0;

#pragma omp parallel
    {
        CountMinSketch thread_cms;
        cms_init_private(&thread_cms, &local_cms);

        uint32_t local_123_private = 0;
        uint32_t local_456_private = 0;
        uint32_t local_range_private = 0;

#pragma omp for
        for (size_t i = 0; i < idx; i++) {
            uint32_t val = local_items[i];
            cms_update_int_parallel(&thread_cms, val, 1);

            if (val == 123) local_123_private++;
            if (val == 456) local_456_private++;
            if (val >= 100 && val <= 110) local_range_private++;
        }

#pragma omp critical
        {
            for (uint32_t d = 0; d < local_cms.depth; d++)
                for (uint32_t w = 0; w < local_cms.width; w++)
                    local_cms.table[d][w] += thread_cms.table[d][w];

            local_cms.total += thread_cms.total;
            local_123 += local_123_private;
            local_456 += local_456_private;
            local_range += local_range_private;
        }

        cms_free_private(&thread_cms);
    }

    t_update_end = MPI_Wtime();
    free(local_items);

    /* --- MPI Reduction --- */
    MPI_Barrier(MPI_COMM_WORLD);
    t_reduce_start = MPI_Wtime();

    CountMinSketch global_cms;
    if (my_rank == 0) {
        global_cms.depth = local_cms.depth;
        global_cms.width = local_cms.width;
        global_cms.total = 0;
        global_cms.hashFunctions =
            malloc(global_cms.depth * sizeof(UniversalHash));
        global_cms.table =
            malloc(global_cms.depth * sizeof(uint32_t*));

        for (uint32_t d = 0; d < global_cms.depth; d++) {
            global_cms.hashFunctions[d] = local_cms.hashFunctions[d];
            global_cms.table[d] =
                calloc(global_cms.width, sizeof(uint32_t));
        }
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

    uint32_t true_123 = 0, true_456 = 0, true_range = 0;
    MPI_Reduce(&local_123, &true_123, 1, MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_456, &true_456, 1, MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_range, &true_range, 1, MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    t_reduce_end = MPI_Wtime();

    // Test queries and timings
    if (my_rank == 0) {
        printf("\n ITEM ESTIMATIONS \n");

        uint32_t est_123 = cms_point_query_int(&global_cms, 123);
        uint32_t est_range = cms_range_query_int(&global_cms, 100, 110);

        printf("Item 123 → estimation: %u, real: %u\n", est_123, true_123);
        printf("Item 456 → estimation: %u, real: %u\n",
               cms_point_query_int(&global_cms, 456), true_456);
        printf("Item 999 → estimation: %u (expected: 0 or small)\n",
               cms_point_query_int(&global_cms, 999));
        printf("Range 100–110 → estimation: %u, real: %u\n",
               est_range, true_range);

        t_end = MPI_Wtime();

        printf("\n TIMINGS \n");
        printf("Total time: %f seconds\n", t_end - t_start);
        printf("I/O + parsing time: %f s\n", t_io_end - t_io_start);
        printf("CMS update time: %f s\n", t_update_end - t_update_start);
        printf("Reduction time: %f s\n", t_reduce_end - t_reduce_start);
        printf("\n --------------------------------------\n");

        cms_free(&global_cms);
    }

    cms_free(&local_cms);
    MPI_Finalize();
    return 0;
}
