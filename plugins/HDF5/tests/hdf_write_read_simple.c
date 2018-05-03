/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 *  This example illustrates how to read/write a subset of data (a slab)
 *  from/to a dataset in an HDF5 file.  It is used in the HDF5 Tutorial.
 */

#include "hdf5.h"
#include <stdio.h>
#include <stdlib.h>

#define FILE        "bert.h5"
#define DATASETNAME "ShortArray"
#define RANK 3
#define RANKW 1

#define DIM0_SUB  1                         /* subset dimensions */
#define DIM1_SUB  1
#define DIM2_SUB  251

#define DIM0     11
#define DIM1     56
#define DIM2     251


int
main (void)
{
    hsize_t     dims[3], dimsm[1];

    hid_t       file_id, dataset_id;        /* handles */
    hid_t       dataspace_id, memspace_id;

    herr_t      status;
    short       data[DIM0][DIM1][DIM2];
    short       wdata[DIM2];

    hsize_t     count[3];              /* size of subset in the file */
    hsize_t     offset[3];             /* subset offset in the file */
    hsize_t     stride[3];
    hsize_t     block[3];
    int         i, j, k;


    /* Create dataset */
    file_id = H5Fcreate (FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    dims[0] = DIM0;
    dims[1] = DIM1;
    dims[2] = DIM2;
    dataspace_id = H5Screate_simple (RANK, dims, NULL);

    dataset_id = H5Dcreate2 (file_id, DATASETNAME, H5T_STD_I16LE, dataspace_id,
                            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    for (i = 0; i < DIM0; i++)
     for (j = 0; j < DIM1; j++)
      for (k = 0; k < DIM2; k++)
	data[i][j][k] = 123 + i*2 + j*3 + k;


    status = H5Dwrite (dataset_id, H5T_NATIVE_SHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);

    printf ("\nFile created.\n");

    status = H5Sclose (dataspace_id);
    status = H5Dclose (dataset_id);
    status = H5Fclose (file_id);


    /*****************************************************
     * Reopen the file and dataset and write a subset of *
     * values to the dataset.
     *****************************************************/

    printf ("Write subset:\n\n");
    file_id = H5Fopen (FILE, H5F_ACC_RDWR, H5P_DEFAULT);
    printf ("H5Fopen returns: %li\n", file_id);
    dataset_id = H5Dopen2 (file_id, DATASETNAME, H5P_DEFAULT);
    printf ("H5Dopen2 returns: %li\n", dataset_id);


    offset[0] = 1;
    offset[1] = 2;
    offset[2] = 0;

    count[0]  = DIM0_SUB;
    count[1]  = DIM1_SUB;
    count[2]  = DIM2_SUB;

    stride[0] = 1;
    stride[1] = 1;
    stride[2] = 1;

    block[0] = 1;
    block[1] = 1;
    block[2] = 1;


    dimsm[0] = DIM2_SUB;
    memspace_id = H5Screate_simple (RANKW, dimsm, NULL);
    printf ("H5Screate_simple returns: %li\n", memspace_id);

    dataspace_id = H5Dget_space(dataset_id);
    printf ("H5Dget_space returns: %li\n", dataspace_id);
    status = H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, offset,
                                  stride, count, block);
    printf ("H5Sselect_hyperslab returns: %li\n", status);

    /* No, no - no overwriting! I do not want to change anything in the file ...
    for (k = 0; k < DIM2_SUB; k++)
	 wdata[k] = 99;

    status = H5Dwrite (dataset_id, H5T_NATIVE_SHORT, memspace_id,
                       dataspace_id, H5P_DEFAULT, wdata);
    printf ("H5Dwrite returns: %li\n", status);
    */

    /* I just need the data in small bits ... */
    status = H5Dread( dataset_id, H5T_NATIVE_SHORT, memspace_id, dataspace_id,
		      H5P_DEFAULT, wdata );
    printf ("H5Dread returns: %li\n", status);
    printf( "Expecting: 141 142 143 144\n" );
    printf( "Read: %hd %hd %hd %hd\n", wdata[10], wdata[11], wdata[12], wdata[13] );

    status = H5Sclose (memspace_id);
    printf ("H5Sclose returns: %li\n", status);
    status = H5Sclose (dataspace_id);
    printf ("H5Sclose returns: %li\n", status);
    status = H5Dclose (dataset_id);
    printf ("H5Dclose returns: %li\n", status);
    status = H5Fclose (file_id);
    printf ("H5Fclose returns: %li\n", status);


}
