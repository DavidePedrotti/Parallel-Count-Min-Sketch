#include "count_min_sketch.h"

// update per un item intero
void cms_update_int(CountMinSketch* cms, int item, int c) {
    cms->total += c;
    for (unsigned int j = 0; j < cms->d; j++) {
        unsigned int hashval = ((long)cms->hashes[j][0]*item + cms->hashes[j][1]) % LONG_PRIME % cms->w;
        cms->C[j][hashval] += c;
    }
}

// Convertire una stringa in un numero intero
unsigned int cms_hashstr(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return (unsigned int)(hash % LONG_PRIME); 
}

// update per stringa
void cms_update_str(CountMinSketch* cms, const char *str, int c) {
    unsigned int hashval = cms_hashstr(str);
    cms_update_int(cms, hashval, c);
}


// point query per intero
unsigned int cms_point_query_int(CountMinSketch* cms, int item) {
    unsigned int min_count = UINT_MAX; // si parte dal massimo valore possibile
    for (unsigned int j = 0; j < cms->d; j++) {
        unsigned int hashval = ((long)cms->hashes[j][0]*item + cms->hashes[j][1]) % LONG_PRIME % cms->w;
        if (cms->C[j][hashval] < min_count) {
            min_count = cms->C[j][hashval];
        }
    }
    return min_count;
}

// point query per stringa
unsigned int cms_point_query_str(CountMinSketch* cms, const char *str) {
    unsigned int hashval = cms_hashstr(str);
    return cms_point_query_int(cms, hashval);
}

unsigned int cms_range_query_int(CountMinSketch* cms, int start, int end) {
    unsigned int total = 0;
    for (int i = start; i <= end; i++) {
        total += cms_point_query_int(cms, i);
    }
    return total;
}

unsigned int cms_range_query_str(CountMinSketch* cms, const char **items, int n) {
    unsigned int total = 0;
    for (int i = 0; i < n; i++) {
        total += cms_point_query_str(cms, items[i]);
    }
    return total;
}
