//
// Created by Christopher Berglund on 11/1/19.
//

#ifndef SIED_HELPERS_H
#define SIED_HELPERS_H
void sort(int *array, int left, int right);
int findClosestValue(int arr[], int l, int r, int x);
int getNeighborBin(int bin, int row, int distance, const int *nBinsInRow, const int *basebins);
int max(const double *arr, int length);
int median(const int *arr, int length);
void get_window(int bin, int row, int width, const int *data, const int *n_bins_in_row,
                const int *basebins, int window[]);
#endif //SIED_HELPERS_H
