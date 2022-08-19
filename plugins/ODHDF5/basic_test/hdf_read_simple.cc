/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "H5Cpp.h"
#include <iostream>

#define FILE        "/data/bert/surveys/F3_Demo_2016_training_v6/Seismics/ORG_420-430_500-600_500-1500_HDF.hdf5"
#define GROUPNAME "Component 1"
#define DATASETNAME "5.3"
#define RANK 3
#define RANKR 1

#define DIM2 1024	    /* Enough to hold last dim in file */

#define DIM0_SUB  1                         /* subset dimensions */
#define DIM1_SUB  1
#define DIM2_SUB  251


int main( int, char** )
{
    hsize_t     dimsm[1];
    short       rdata[DIM2];

    hsize_t     count[3];              /* size of subset in the file */
    hsize_t     offset[3];             /* subset offset in the file */
    hsize_t     stride[3];

    offset[0] = 1;
    offset[1] = 2;
    offset[2] = 0;

    count[0]  = DIM0_SUB;
    count[1]  = DIM1_SUB;
    count[2]  = DIM2_SUB;

    stride[0] = 1;
    stride[1] = 1;
    stride[2] = 1;

    dimsm[0] = DIM2_SUB;

    try {

	H5::H5File h5file( FILE, H5F_ACC_RDONLY );
	H5::Group group = h5file.openGroup( GROUPNAME );
	H5::DataSet dataset = group.openDataSet( DATASETNAME );
	H5::DataSpace filedataspace = dataset.getSpace();
	filedataspace.selectHyperslab( H5S_SELECT_SET,
					count, offset, stride );
	H5::DataSpace memdataspace( RANKR, dimsm );
	dataset.read( rdata, H5::PredType::STD_I16LE, memdataspace,
						      filedataspace );
    }
    catch ( H5::Exception& exc ) {
	std::cerr << exc.getCDetailMsg() << std::endl;
	return 1;
    }
    catch ( ... ) {
	std::cerr << "Unexpected error" << std::endl;
	return 1;
    }

    std::cout << "Expecting: -182 -380 -436 -2911" << std::endl;
    std::cout << "Read: " << rdata[10] << ' ' << rdata[11] << ' ' << rdata[12]
			  << ' ' << rdata[13] << std::endl;
}
