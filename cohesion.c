//
// Created by Christopher Berglund on 11/1/19.
//

#include <stdbool.h>
#include <stdlib.h>
#include "cohesion.h"

#define CRIT_C1 0.90
#define CRIT_C2 0.90
#define CRIT_C 0.92

bool isCohesive(const int *window, int width, int threshold) {
    int *copy = malloc((width * width) * sizeof(int));
    for (int i = 0; i < width * width; i++) {
        copy[i] = window[i] >= threshold;
    }
    double r1 = 0;
    double t1 = 0;
    double r2 = 0;
    double t2 = 0;
    for (int i = 1; i < width - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            if (window[i * width + j] == 0) {
                if (window[i * width + j] == window[(i - 1) * width + j]) r1++;
                if (window[i * width + j] == window[(i + 1) * width + j]) r1++;
                if (window[i * width + j] == window[i * width + (j - 1)]) r1++;
                if (window[i * width + j] == window[i * width + (j + 1)]) r1++;
                t1 += 4;
            } else if (window[i * width + j] == 1) {
                if (window[i * width + j] == window[(i - 1) * width + j]) r2++;
                if (window[i * width + j] == window[(i + 1) * width + j]) r2++;
                if (window[i * width + j] == window[i * width + (j - 1)]) r2++;
                if (window[i * width + j] == window[i * width + (j + 1)]) r2++;
                t2 += 4;
            }
        }
    }
    double c1 = r1/t1;
    double c2 = r2/t2;
    double c = (r1 + r2)/(t1 + t2);
    free(copy);
    return (c1 >= CRIT_C1 && c2 >= CRIT_C2 && c >= CRIT_C);
}