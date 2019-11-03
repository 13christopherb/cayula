#include <stdlib.h>
#include <stdio.h>
#include "histogram.h"
#include "prefilter.h"
#include "helpers.h"
#include "cohesion.h"
#include "locator.h"
#include "contour.h"
#include "filter.h"
#include "cayula.h"

void cayula(int totalBins, int nDataBins, int nrows, int fillValue,
            int *dataBins, int *rows, double *inData, double *weights, double *lats, double *lons, int *outData,
            bool chlora) {
    int *bins = (int *) malloc(totalBins * sizeof(int));
    int *nBinsInRow = (int *) malloc(nrows * sizeof(int));
    int *basebins = (int *) malloc(nrows * sizeof(int));
    int *data = (int *) malloc(totalBins * sizeof(int));
    printf("Start \n");
    createFullBinArray(totalBins, nDataBins, nrows, dataBins, fillValue,
                       bins, inData, weights, lats, lons, nBinsInRow, basebins, data, chlora);
    printf("Initializing done \n");
    free(inData);
    int *filteredData = (int *) malloc(totalBins * sizeof(int));
    medianFilter(bins, data, filteredData, totalBins, nrows, nBinsInRow, basebins, fillValue);
    printf("Median filter done \n");

    //int *edgePixels = (int *) malloc(totalBins * sizeof(int));
    for (int i = 15; i < nrows - 16; i += 16) {
        for (int j = 15; j < nBinsInRow[i] - 16; j += 16) {
            printf("%d \n", i);
            int *window = (int *) malloc(1024 * sizeof(int));
            int *binWindow = (int *) malloc(1024 * sizeof(int));
            getWindow(basebins[i] + j, i, 32, data, nBinsInRow, basebins, window, fillValue, false);
            getWindow(basebins[i] + j, i, 32, bins, nBinsInRow, basebins, binWindow, fillValue, false);
            int threshold = histogramAnalysis(window, 32, 255);
            /*if (threshold > 0) {
                if (isCohesive(window, 32, threshold)) {
                    int *edgeWindow = malloc(1024 * sizeof(int));
                    locateEdgePixels(window, edgeWindow, 32, threshold);
                    for (int k = 0; k < 32; k++) {
                        for (int m = 0; m < 32; m++) {
                            if (window[i * 32 + j] == threshold)
                                edgePixels[binWindow[i * 32 + j] - 1] = edgeWindow[i * 32 + j];
                        }
                    }
                    free(edgeWindow);
                }
            }*/
            free(window);
            free(binWindow);
        }
    }
    free(data);
    printf("Edgeing done \n");
    //contour(bins, edgePixels, outData, totalBins, nrows, nBinsInRow, basebins, fillValue);
    printf("Contouring done \n");
    free(bins);
    free(nBinsInRow);
    free(basebins);
}

