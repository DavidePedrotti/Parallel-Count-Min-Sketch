#ifndef COUNT_MIN_SKETCH_H
#define COUNT_MIN_SKETCH_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>   // per UINT_MAX

#define LONG_PRIME 4294967311UL // usato per migliorare la distribuzione degli hash

typedef struct {
    unsigned int w;     // width
    unsigned int d;     // depth
    unsigned int total; // totale conteggi
    int **C;            // array contatori d x w
    int **hashes;       // array parametri hash {a,b}
} CountMinSketch;

// update per un item intero
void cms_update_int(CountMinSketch* cms, int item, int c);

// hash semplice per stringa (djb2)
unsigned int cms_hashstr(const char *str);

// update per stringa
void cms_update_str(CountMinSketch* cms, const char *str, int c);

// point query per intero
unsigned int cms_point_query_int(CountMinSketch* cms, int item);

// point query per stringa
unsigned int cms_point_query_str(CountMinSketch* cms, const char *str);

// range query per intero
unsigned int cms_range_query_int(CountMinSketch* cms, int start, int end);

// range query per stringa
unsigned int cms_range_query_str(CountMinSketch* cms, const char **items, int n);


#endif // COUNT_MIN_SKETCH_H
