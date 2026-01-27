#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "count_min_sketch.h"

/*
 * Parallel Count-Min Sketch (MAINV2)
 * MPI-I/O safe for large files
 */

int main(int argc, char* argv[]) {
    int comm_sz, my_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    MPI_Barrier(MPI_COMM_WORLD);
    double t_start = MPI_Wtime();

    srand(time(NULL) + my_rank);
    const char* FILENAME = argv[1];

    //  CMS initialization 
    CountMinSketch local_cms;
    if (cms_init(&local_cms, EPSILON, DELTA, PRIME) != 0) {
        if (my_rank == 0)
            fprintf(stderr, "Error initializing CMS\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Bcast(local_cms.hashFunctions,
              local_cms.depth * sizeof(UniversalHash),
              MPI_BYTE, 0, MPI_COMM_WORLD);

    // ---------------- MPI-I/O ----------------
    MPI_Barrier(MPI_COMM_WORLD);
    double t_io_start = MPI_Wtime();

    MPI_File fh;
    MPI_File_open(MPI_COMM_WORLD, FILENAME, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);

    MPI_Offset file_size;
    MPI_File_get_size(fh, &file_size);

    MPI_Offset approx_chunk = file_size / comm_sz;
    MPI_Offset my_start = my_rank * approx_chunk;
    MPI_Offset my_end = (my_rank == comm_sz - 1) ? (file_size - 1) : (my_start + approx_chunk - 1);

    if (my_rank != 0) {
        char c;
        MPI_File_read_at(fh, my_start - 1, &c, 1, MPI_CHAR, MPI_STATUS_IGNORE);
        while (c != '\n' && my_start < my_end) {
            my_start++;
            MPI_File_read_at(fh, my_start - 1, &c, 1, MPI_CHAR, MPI_STATUS_IGNORE);
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
        MPI_File_read_at(fh, offset, ptr, to_read, MPI_CHAR, MPI_STATUS_IGNORE);
        remaining -= to_read;
        offset += to_read;
        ptr += to_read;
    }
    buffer[my_chunk_size] = '\0';

    if (my_rank != comm_sz - 1) {
        char* last_nl = strrchr(buffer, '\n');
        if (last_nl) *(last_nl + 1) = '\0';
        else buffer[0] = '\0';
    }

    MPI_File_close(&fh);

    // Count lines in local chunk
    size_t local_line_count = 0;
    for (char* p = buffer; *p; p++)
        if (*p == '\n') local_line_count++;

    size_t total_items = 0;
    MPI_Reduce(&local_line_count, &total_items, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    // Rank 0 prints dataset info
    if (my_rank == 0) {
        printf("Parallel Count-Min Sketch (MAINV2)\n");

        // Ottieni dimensione reale del file su disco
        FILE* fp_check = fopen(FILENAME, "rb");
        if (!fp_check) {
            fprintf(stderr, "Cannot open file %s to check size\n", FILENAME);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        fseek(fp_check, 0, SEEK_END);
        long fsize = ftell(fp_check);
        fclose(fp_check);

        printf("\n DATASET INFO \n");
        printf("Dataset file: %s\n", FILENAME);
        double file_size_mb = (double)fsize / (1024.0 * 1024.0);
        printf("File size on disk: %.2f MB\n", file_size_mb);
    }

    uint32_t* local_items = malloc(local_line_count * sizeof(uint32_t));
    if (!local_items) MPI_Abort(MPI_COMM_WORLD, 99);

    // Parse numbers
    size_t idx = 0;
    char* token = strtok(buffer, "\n");
    while (token) {
        local_items[idx++] = (uint32_t)strtoul(token, NULL, 10);
        token = strtok(NULL, "\n");
    }
    free(buffer);

    MPI_Barrier(MPI_COMM_WORLD);
    double t_io_end = MPI_Wtime();

    //  CMS update + local counters 
    MPI_Barrier(MPI_COMM_WORLD);
    double t_update_start = MPI_Wtime();

    uint32_t local_123 = 0, local_456 = 0, local_range = 0;

    for (size_t i = 0; i < idx; i++) {
        uint32_t val = local_items[i];
        cms_update_int(&local_cms, val, 1);

        if (val == 123) local_123++;
        if (val == 456) local_456++;
        if (val >= 100 && val <= 110) local_range++;
    }
    free(local_items);

    MPI_Barrier(MPI_COMM_WORLD);
    double t_update_end = MPI_Wtime();

    //  Reduction 
    MPI_Barrier(MPI_COMM_WORLD);
    double t_reduce_start = MPI_Wtime();

    CountMinSketch global_cms;
    if (my_rank == 0) {
        global_cms.depth = local_cms.depth;
        global_cms.width = local_cms.width;
        global_cms.total = 0;
        global_cms.hashFunctions = malloc(global_cms.depth * sizeof(UniversalHash));
        global_cms.table = malloc(global_cms.depth * sizeof(uint32_t*));
        for (uint32_t d = 0; d < global_cms.depth; d++) {
            global_cms.hashFunctions[d] = local_cms.hashFunctions[d];
            global_cms.table[d] = calloc(global_cms.width, sizeof(uint32_t));
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
               1, MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);

    uint32_t true_123 = 0, true_456 = 0, true_range = 0;
    MPI_Reduce(&local_123, &true_123, 1, MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_456, &true_456, 1, MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_range, &true_range, 1, MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    double t_reduce_end = MPI_Wtime();

    if (my_rank == 0) {
        printf("\n--- ITEM ESTIMATIONS ---\n");
        printf("Item 123 → estimation: %u, real: %u\n", cms_point_query_int(&global_cms, 123), true_123);
        printf("Item 456 → estimation: %u, real: %u\n", cms_point_query_int(&global_cms, 456), true_456);
        printf("Item 999 → estimation: %u (expected: 0 or a small number)\n", cms_point_query_int(&global_cms, 999));

        printf("\nStart Test: Range Query\n");
        printf("Range 100–110 → estimation: %u, real: %u\n", cms_range_query_int(&global_cms, 100, 110), true_range);

        size_t batch_size = 1000000;
        double pq_start = MPI_Wtime();
        for (size_t i = 0; i < batch_size; i++)
            (void) cms_point_query_int(&global_cms, 123);
        double pq_end = MPI_Wtime();

        double rq_start = MPI_Wtime();
        for (size_t i = 0; i < batch_size; i++)
            (void) cms_range_query_int(&global_cms, 100, 110);
        double rq_end = MPI_Wtime();

        double ip_start = MPI_Wtime();
        for (size_t i = 0; i < batch_size; i++)
            (void) cms_inner_product(&global_cms, &global_cms);
        double ip_end = MPI_Wtime();

        printf("\n--- TIMINGS ---\n");
        printf("Total time: %f seconds\n", t_reduce_end - t_start);
        printf("I/O + parsing time: %f s\n", t_io_end - t_io_start);
        printf("CMS update time: %f s\n", t_update_end - t_update_start);
        printf("Reduction time: %f s\n", t_reduce_end - t_reduce_start);
        printf("Point query time: %e s\n", (pq_end - pq_start)/batch_size);
        printf("Range query time: %e s\n", (rq_end - rq_start)/batch_size);
        printf("Inner product time: %e s\n", (ip_end - ip_start)/batch_size);
        printf("\n --------------------------------------\n");

        cms_free(&global_cms);
    }

    cms_free(&local_cms);
    MPI_Finalize();
    return 0;
}
