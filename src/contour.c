#include "contour.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "helpers.h"
#include "cayula.h"

static inline double square(double a) {
    return a * a;
}

static inline int mod(int a, int n) {
    return a - floor(a / n) * n;
}


const int ANGLES[9] = {135, 90, 45,
                       180, 360, 0,
                       225, 270, 315};

struct contour {
    struct contour_point *first_point;
    struct contour *prev;
    struct contour *next;
    int length;
} typedef Contour;

struct vector{
    double x;
    double y;
} typedef Vector;


static inline int dot(Vector a, Vector b) {
    return a.x * b.x + a.y * b.y;
}
/*
 * Function:  gradient
 * --------------------
 * Calculates the gradient of the center pixel in the provided array using first order central differences. Fill values
 * at pixels within the directional step of the gradient calculation are replaced with the center pixel value.
 *
 * args:
 *      int *window: pointer to an array of 9 elements representing a window of data values from which to calculate
 *      the gradient of the center pixel
 *
 * returns:
 *      Vector: the vector gradient
 */
Vector gradient(int *window) {
    if (window[5] == FILL_VALUE) window[5] = window[4];
    if (window[3] == FILL_VALUE) window[3] = window[4];
    if (window[7] == FILL_VALUE) window[7] = window[4];
    if (window[1] == FILL_VALUE) window[1] = window[4];
    Vector g;
    g.x = (double) (window[5] - window[3]) / 2;
    g.y = (double) (window[7] - window[1]) / 2;
    return g;
}

/*
 * Function:  turn_too_sharp
 * --------------------
 * Checks to see if adding a point at the provided angle would result in the contour changing direction by more than 90
 * degrees over the course of 5 pixels.
 *
 * args:
 *      ContourPoint *tail: the last point currently on the contour
 *      int next_theta: the angle between the last point on the contour and the point to be added
 *
 * returns:
 *      int: 1 if change in direction exceeds 90 degrees, 0 if it does not
 */
int turn_too_sharp(ContourPoint *tail, int next_theta) {
    int counter = 0;
    int is_turn_too_sharp = 0;
    ContourPoint *tmp = tail;
    while (tmp->prev->prev != NULL && counter < 5) {
        int dtheta = mod(tmp->angle - next_theta + 180, 360) - 180;
        dtheta = dtheta < -180 ? dtheta + 360 : dtheta;
        dtheta = abs(dtheta);
        if (dtheta > 90) {
            is_turn_too_sharp = 1;
            break;
        }
        tmp = tmp->prev;
        counter++;

    }
    return is_turn_too_sharp;
}

/*
 * Function:  get_gradient_ratio
 * --------------------
 * Calculates the ratio between the magnitude of the gradient sum and the sum of magnitude of the gradient of the pixels
 * in a 3x3 window centered on the center pixel of the provided 5x5 window. The gradient is calculated with
 * first order central differences.
 *
 * args:
 *      int *window: pointer to an array 25 elements long representing a 5x5 window centered on the last contour pixel
 *
 * returns:
 *      int: the ratio between the magnitude of the sum of the gradient vectors and the sum of the magnitude of the
 *      gradient vectors for each pixel in a 3x3 window centered on the center pixel of the provided window.
 */
double gradient_ratio(const int *window) {
    double sum_magnitude = 0, sum_x = 0, sum_y = 0;
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int inner_window[9];
            inner_window[0] = window[i * 5 + 6 + j];
            inner_window[1] = window[i * 5 + 7 + j];
            inner_window[2] = window[i * 5 + 8 + j];
            inner_window[3] = window[i * 5 + 11 + j];
            inner_window[4] = window[i * 5 + 12 + j];
            inner_window[5] = window[i * 5 + 13 + j];
            inner_window[6] = window[i * 5 + 16 + j];
            inner_window[7] = window[i * 5 + 17 + j];
            inner_window[9] = window[i * 5 + 18 + j];

            Vector g = gradient(inner_window);
            sum_magnitude += sqrt(square(g.x) + square(g.y));
            sum_x += g.x;
            sum_y += g.y;
        }
    }
    return sqrt(square(sum_x) + square(sum_y)) / sum_magnitude;
}

 Contour *new_contour(Contour *prev, int bin) {
     Contour *n = malloc(sizeof(Contour));
     n->prev = prev;
     n->next = NULL;
     n->length = 1;
     ContourPoint *c = malloc(sizeof(ContourPoint));
     c->bin = bin;
     c->prev = NULL;
     c->next = NULL;
     n->first_point = c;
     return n;
 }

/*
* Function:  del_contour
* --------------------
* Deletes the provided node in the linked list of contours and all the contour point nodes that belongs to it. If
* the node is the middle of the linked list, the nodes before and after are linked to each other.
*
* args:
*      Contour *n: the node of the linked list to delete
*
*/
void del_contour(Contour *n) {
    ContourPoint *c = n->first_point;
    while (c->next != NULL) c = c->next;
    while (c->prev != NULL) {
        ContourPoint *tmp = c->prev;
        free(c);
        c = tmp;
    }
    if (n->prev != NULL) {
        if (n->next != NULL) {
            n->prev->next = n->next;
        } else {
            n->prev = NULL;
        }
    }
    free(n);
}

/*
 * Function:  new_contour_point
 * --------------------
 * Creates a new contour point and adds it to the end of the doubly linked list representing a single contour.
 *
 * args:
 *      ContourPoint *prev: the last node in the contour linked list
 *      int bin: the bin number of the new point to add to the list
 *      int angle: the angle between the last point in the contour and the new point
 *
 * returns:
 *      ContourPoint *: the new point that is now the last node in the linked list
 */
ContourPoint * new_contour_point(ContourPoint *prev, int bin, int angle) {
    ContourPoint *c = malloc(sizeof(ContourPoint));
    prev->next = c;
    c->bin = bin;
    c->angle = angle;
    c->prev = prev;
    c->next = NULL;
    return c;
}

/*
 * Function:  get_bin_number
 * --------------------
 * Returns a bin number provided the index of a 9 element array representing a 3x3 window centered on the given bin
 * number.
 *
 * args:
 *      int bin: the center bin number in the window of interest
 *      int i: the index for the desired bin in the window of interest
 *      int row: the row number of the center bin
 *      int *basebins: pointer to an array containing the bin number for the first bin of each row
 *      int *nbins_in_row: pointer to an array containing the number of bins in each row
 *
 * returns:
 *      int: bin number of the desired bin
 */
static int get_bin_number(int bin, int i, int row, const int *basebins, const int *nbins_in_row) {
    double ratio;
    int next_bin;
    switch(i) {
        case 0:
            ratio = (bin - basebins[row]) / (double) nbins_in_row[row];
            next_bin = (int) (ratio * nbins_in_row[row - 1] + 0.5)  + basebins[row - 1] - 1;
            break;
        case 1:
            ratio = (bin - basebins[row]) / (double) nbins_in_row[row];
            next_bin = (int) (ratio * nbins_in_row[row - 1] + 0.5)  + basebins[row - 1];
            break;
        case 2:
            ratio = (bin - basebins[row]) / (double) nbins_in_row[row];
            next_bin = (int) (ratio * nbins_in_row[row - 1] + 0.5)  + basebins[row - 1] + 1;
            break;
        case 3:
            next_bin = bin - 1;
            break;
        case 5:
            next_bin = bin + 1;
        case 6:
            ratio = (bin - basebins[row]) / (double) nbins_in_row[row];
            next_bin = (int) (ratio * nbins_in_row[row + 1] + 0.5)  + basebins[row + 1] - 1;
            break;
        case 7:
            ratio = (bin - basebins[row]) / (double) nbins_in_row[row];
            next_bin = (int) (ratio * nbins_in_row[row + 1] + 0.5)  + basebins[row + 1];
            break;
        case 8:
            ratio = (bin - basebins[row]) / (double) nbins_in_row[row];
            next_bin = (int) (ratio * nbins_in_row[row + 1] + 0.5)  + basebins[row + 1] + 1;
            break;
    }
    return next_bin;
}

/*
 * Function:  find_best_front
 * --------------------
 * Of the bins neighboring the last bin on the contour, this function selects the best front bin to add to the contour.
 * Going through all the neighboring bins, the function identifies the next bin that will change the direction of the
 * contour the least. However, if adding the selected bin would result in the contour changing direction by more than
 * 90 degrees over the course of 5 bins, the bin is rejected as a possible addition to the contour. If the provided
 * contour point is the first point in the contour and thus has no direction, the selection is biased towards higher
 * numbered bins as they are the least likely to be contained in other contours.
 *
 * args:
 *      ContourPoint *prev: the last edge pixel in the current contour
 *      int *data: pointer to a boolean array representing the pixels status as an edge pixel
 *      int row: the row of the last edge pixel in the current contour
 *      int *basebins: pointer to an array containing the index of the first bin of each row
 *      int *nbins_in_row: pointer to an array containing the number of bins in each row
 *
 * returns:
 *      ContourPoint *: the selected point to add to the contour. Pointer will be NULL if there is no previously
 *      identified edge pixel to add to the contour.
 */
ContourPoint * find_best_front(ContourPoint *prev, const int *data,  int row, const int *basebins, const int *nbins_in_row) {
    int edge_window[9];
    get_window(prev->bin, row, 3, data, nbins_in_row, basebins, edge_window);
    int next_bin = -1;
    int min_dtheta = 180;
    int next_angle;
    for (int i = 0; i < 9; i++) {
        int dtheta = 180;
        if (i != 4 && edge_window[i]) {
            if (prev->prev == NULL) {
                dtheta = 0;
            } else {
                dtheta = prev->angle - ANGLES[i];
                dtheta = mod(dtheta + 180, 360) - 180;
                dtheta = dtheta < -180 ? dtheta + 360 : dtheta;
                dtheta = abs(dtheta);
            }
            if (dtheta == 0 || dtheta < min_dtheta) {
                min_dtheta = dtheta;
                next_angle = ANGLES[i];
                next_bin = get_bin_number(prev->bin, i, row, basebins, nbins_in_row);
            }
        }
    }

    if (next_bin != -1 && (prev->prev == NULL || !turn_too_sharp(prev, next_angle))) {
        return new_contour_point(prev, next_bin, next_angle);
    } else {
        return NULL;
    }
}

int follow_contour(ContourPoint *prev, const int *data, const int *filtered_data, int *pixel_in_contour, int row, const int *basebins, const int *nbins_in_row) {
    ContourPoint *next_point;
    next_point = find_best_front(prev, data, row, basebins,nbins_in_row);
    int count = 1;
    double ratio = 0;
    int max_bin;
    if (next_point == NULL) {
        int outer_window[25];
        get_window(prev->bin, row, 5, filtered_data, nbins_in_row, basebins, outer_window);
        double ratio = gradient_ratio(outer_window);
        if (ratio > 0.7) {
            int bin_window[9];
            double max_product = -1;
            Vector v;
            int max_idx = -1;
            get_window(prev->bin, row, 3, filtered_data, nbins_in_row, basebins, bin_window);
            Vector gradient0 = gradient(bin_window);
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    if (i != 1 && j != 1) {
                        int bin = get_bin_number(prev->bin, i * 3 + j, row,basebins, nbins_in_row);
                        get_window(bin, row + j - 1, 3, filtered_data, nbins_in_row, basebins, bin_window);
                        Vector gradient1 = gradient(bin_window);
                        double product = dot(gradient0, gradient1);
                        if (product > max_product) {
                            max_product = product;
                            max_idx = i * 3 + j;
                            max_bin = bin;
                        }
                    }

                }
            }
            if (max_product > 0) {
                next_point = new_contour_point(prev, max_bin, ANGLES[max_idx]);
            }
        }
    }

    if (next_point != NULL && !pixel_in_contour[next_point->bin]) {
        int next_row;
        pixel_in_contour[next_point->bin] = 1;
        switch(next_point->angle) {
            case 0:
            case 180:
                next_row = row;
                break;
            case 1 ... 179:
                next_row = row - 1;
                break;
            case 181 ... 359:
                next_row = row + 1;
                break;
            default:
                next_row = row;
                break;
        }
        count += follow_contour(next_point, data, filtered_data, pixel_in_contour, next_row, basebins, nbins_in_row);
    }

    return count;
}
/*
void clearSubList(struct node *head) {
    struct subnode *subhead = head->child;
    struct subnode *tmp = NULL;
    head->child = NULL;
    while (subhead != NULL) {
        tmp = subhead->next;
        free(subhead);
        subhead = tmp;
    }
}*/
/*
void trim(struct node *head) {
    struct node *current = head;
    struct node *next;
    struct node *prev;
    while (current != NULL) {
        if (current->length < 4) {
            clearSubList(current);
            prev = current->prev;
            next = current->next;
            if (prev != NULL) {
                prev->next = next;
                if (next != NULL) {
                    next->prev = prev;
                }
            }
            if (current == head) {
                head = NULL;
                free(head);
            } else
                free(current);
            current = next;
        } else {
            current = current->next;
        }
    }
}
*/


void contour(int *data, int *filtered_data, int nbins, int nrows, const int *nbins_in_row, const int *basebins) {
    int *pixel_in_contour = malloc(sizeof(int) * nbins);
    memset(pixel_in_contour, 0, nbins * sizeof(int));
    Contour *contours = NULL;
    for (int i = 0; i < nrows; i++) {
        for (int j = basebins[i]; j < basebins[i] + nbins_in_row[i]; j++) {
            if (data[j] && !pixel_in_contour[j]) {
                pixel_in_contour[j] = 1;
                if (contours == NULL) {
                    contours = new_contour(NULL, j);
                } else {
                    contours->next = new_contour(contours, j);
                    contours = contours->next;
                }

                //TODO Grow contour
            }
        }
    }
    //TODO Kill the contours that are too short
}

/**
 * Implements a contour following algorithm. Goes through each bin, and if the bin has been previously detected as
 * an edge, then follow neighboring bin
 * @param bins
 * @param inData
 * @param outData
 * @param ndata
 * @param nrows
 * @param nBinsInRow
 * @param basebins
 * @param fillValue
 */
 /*
void contour2(int *bins, int *inData, int *filteredData, int *outData, int ndata, int nrows, const int *nBinsInRow,
             const int *basebins, int fillValue) {
    int row = 0;
    struct node *head = NULL;
    struct node *tail = NULL;
    struct node *tmp = NULL;
    head = (struct node *) malloc(sizeof(struct node));
    tail = head;
    head->child = NULL;
    head->next = NULL;
    head->length = 0;
    head->prev = NULL;
    bool *isInContour = (bool *) malloc(sizeof(bool) * ndata);
    for (int i = 0; i < ndata; i++) {
        isInContour[i] = false;
    }
    for (int i = 0; i < ndata; i++) {
        if (row < 2 || row > nrows - 3) {
            if (i == basebins[row] + nBinsInRow[row] - 1) {
                row++;
            }
            continue;
        }

        if (inData[i] > 0 && !isInContour[i]) {
            isInContour[i] = true;
            if (tail->child != NULL) {
                tmp = (struct node *) malloc(sizeof(struct node));
                tail->next = tmp;
                tmp->prev = tail;
                tail = tmp;
            }
            tail->child = (struct subnode *) malloc(sizeof(struct subnode));
            tail->next = NULL;
            tail->child->prev = NULL;
            tail->child->next = NULL;
            tail->child->entryAngle = -999;
            tail->child->bin = i + 1;
            tail->length = followContour(i + 1, row, bins, inData, filteredData, isInContour, nBinsInRow, basebins,
                                         fillValue, tail->child);
        }
        if (i == basebins[row] + nBinsInRow[row] - 1) {
            row++;
        }
    }
    free(isInContour);
    struct node *current = head;
    struct subnode *child = NULL;
    struct subnode *tmpchild = NULL;
    trim(head);
    while (current != NULL) {
        child = current->child;
        while (child != NULL) {
            outData[(child->bin) - 1] = 1;
            tmpchild = child->next;
            free(child);
            child = tmpchild;
        }
        tmp = current->next;
        free(current);
        current = tmp;
    }
}*/