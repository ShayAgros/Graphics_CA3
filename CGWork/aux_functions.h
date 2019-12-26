/* This file includes declarions for auxiliary functions
 */

#pragma once

/* Perform mergeSort on an array of lines. Lines
 * l1, l2 satisfy l1 < l2 if the minimum y value of l1
 * is smaller than l2's.
 */
void mergeSort(struct twod_line arr[], int sz);

/* Perform bucketSort on an array, using the given min_val and max_val
 * a as values boundries. The sorting also removes duplications of the
 * same value (same as unique in *nix machines)
 */
void bucketSortAndUnique(int arr[], int &arr_sz, int min_val, int max_val);