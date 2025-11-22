#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include "count_min_sketch.h"

#define MAX_LINE_LEN 64
#define MAX_ITEMS_PER_RANK 1000

int main(int argc, char* argv[]) {
    int comm_sz, my_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    srand(time(NULL) + my_rank);
    if(my_rank == 0) {
        printf("Parallel Count-Min Sketch with MPI-I/O\n");
    }

    CountMinSketch local_cms;
    if (cms_init(&local_cms, EPSILON, DELTA, PRIME) != 0) {
        if (my_rank == 0) fprintf(stderr, "Error in cms_init\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Bcast(local_cms.hashFunctions,local_cms.depth * sizeof(UniversalHash), MPI_BYTE, 0, MPI_COMM_WORLD);

    const char* FILENAME = "dataset.txt";

    uint32_t true_123 = 0, true_456 = 0, true_range = 0;

    if (my_rank == 0) {
        FILE* fp = fopen(FILENAME, "r");
        if (!fp) {
            fprintf(stderr, "Rank 0: cannot open file %s\n", FILENAME);
            MPI_Abort(MPI_COMM_WORLD, 2);
        }
        char line[MAX_LINE_LEN];
        while (fgets(line, MAX_LINE_LEN, fp)) {
            uint32_t v = (uint32_t)atoi(line);
            if (v == 123) true_123++;
            if (v == 456) true_456++;
            if (v >= 100 && v <= 110) true_range++;
        }
        fclose(fp);
    }

    // MPI-I/O: each rank reads its chunk of the file
    MPI_File fh;
    MPI_File_open(MPI_COMM_WORLD, FILENAME, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);

    MPI_Offset file_size;
    MPI_File_get_size(fh, &file_size);
    file_size--; // exclude EOF

    MPI_Offset approx_chunk = file_size / comm_sz;
    MPI_Offset my_start = my_rank * approx_chunk;

    if (my_rank != 0) {
        MPI_File_seek(fh, my_start - 1, MPI_SEEK_SET);
        char c;
        MPI_File_read(fh, &c, 1, MPI_CHAR, MPI_STATUS_IGNORE);
        while (c != '\n') {
            MPI_File_read(fh, &c, 1, MPI_CHAR, MPI_STATUS_IGNORE);
            my_start++;
            if (my_start >= file_size) break;
        }
    }

    MPI_File_seek(fh, my_start, MPI_SEEK_SET);

    uint32_t* local_items = malloc(MAX_ITEMS_PER_RANK * sizeof(uint32_t));
    if (!local_items) {
        fprintf(stderr, "Rank %d: malloc failed\n", my_rank);
        MPI_Abort(MPI_COMM_WORLD, 99);
    }

    int read_count = 0;
    char line[MAX_LINE_LEN];
    MPI_Offset curr_pos;
    while (read_count < MAX_ITEMS_PER_RANK) {
        int idx = 0;
        char c;

        do {
            MPI_File_read(fh, &c, 1, MPI_CHAR, MPI_STATUS_IGNORE);
            if (c == '\n' || idx == MAX_LINE_LEN - 1) 
                break;
            line[idx++] = c;
        } while (1);

        line[idx] = '\0';
        if (idx > 0) local_items[read_count++] = (uint32_t)atoi(line);

        // checks if we have reached the end
        MPI_File_get_position(fh, &curr_pos);
        if (curr_pos >= file_size) break;
    }
    MPI_File_close(&fh);

    for (int i = 0; i < read_count; i++)
        cms_update_int(&local_cms, local_items[i], 1);

    free(local_items);

  
    CountMinSketch global_cms;
    if (my_rank == 0) {
        if (cms_init(&global_cms, EPSILON, DELTA, PRIME) != 0) {
            fprintf(stderr, "Error initializing global_cms\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
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
        test_basic_update_query(&global_cms, true_123, true_456);
        test_range_query(&global_cms, true_range);

        cms_free(&global_cms);
    }

    cms_free(&local_cms);
    MPI_Finalize();
    return 0;
}

