/* This file includes auxiliary functions which are used by the IritObjects classes
 */

#include "aux_functions.h"
#include "IritObjects.h"

void merge(struct twod_line arr[], int l, int m, int r, struct twod_line helper[]);

// Merges two subarrays of arr[]. 
// First subarray is arr[l..m] 
// Second subarray is arr[m+1..r-1] 
void merge(struct twod_line arr[], int l, int m, int r, struct twod_line helper[])
{
	int i, j, k;
	int n1 = m - l + 1;
	int n2 = r - (m + 1);


	//for (i = 0; i < n1 + n2; i++)
	//	helper[l + i] = arr[l + i];
	for (i = 0; i < n1; i++)
		helper[l + i] = arr[l + i];
	for (j = 0; j < n2; j++)
		helper[m + 1 + j] = arr[m + 1 + j];

	/* Merge the temp arrays back into arr[l..r-1] */
	i = 0; // Initial index of first subarray 
	j = 0; // Initial index of second subarray 
	k = l; // Initial index of merged subarray 
	while (i < n1 && j < n2)
	{
		if ( helper[l+i] < helper[m+1+j])
		{
			arr[k] = helper[l + i];
			i++;
		}
		else
		{
			arr[k] = helper[m + 1 + j];
			j++;
		}
		k++;
	}

	/* Copy the remaining elements , if there
	   are any */
	while (i < n1)
	{
		arr[k] = helper[l + i];
		i++;
		k++;
	}

	/* Copy the remaining elements, if there
	   are any */
	while (j < n2)
	{
		arr[k] = helper[m + 1 + j];
		j++;
		k++;
	}
}

/* l is for left index and r is right index of the
   sub-array of arr to be sorted */
void mergeSort_aux(struct twod_line arr[], int l, int r, struct twod_line helper[])
{
	if (l < r - 1)
	{
		// Same as (l+r)/2, but avoids overflow for 
		// large l and h 
		int m = l + (int)((r - l) / 2);

		// Sort first and second halves
		mergeSort_aux(arr, l, m, helper);
		mergeSort_aux(arr, m, r, helper);

		merge(arr, l, m - 1, r, helper);
	}
}

void mergeSort(struct twod_line arr[], int sz)
{
	struct twod_line *helper = new struct twod_line[sz]();

	mergeSort_aux(arr, 0, sz, helper);

	delete[] helper;
}

void bucketSortAndUnique(int arr[], int &arr_sz, int min_val, int max_val)
{
	int offset = min_val;
	int *bucket_arr;
	int i, j;

	// We "move" all values -min_val in order to have the first value
	// at 0. This is done to save allocation space
	min_val = 0;
	max_val -= offset;

	// If max = 0 than all the values in the array are the same. No need
	// to sort
	if (!max_val)
		return;

	bucket_arr = new int[max_val + 1];
	
	// zero bucket array
	for (i = 0; i < max_val + 1; i++) {
		bucket_arr[i] = 0;
	}

	// fill bucket with the number of iteration for each value
	for (i = 0; i < arr_sz; i++) {
		//bucket_arr[arr[i] - offset]++;
		bucket_arr[arr[i] - offset] = 1;
	}

	arr_sz = 0;
	for (i = 0; i < max_val + 1; i++) {
		if (bucket_arr[i] > 2) {
			printf("Stop\n");
		}
		for (j = 0; j < bucket_arr[i]; j++)
			arr[arr_sz++] = i + offset;
	}

	delete[] bucket_arr;
}