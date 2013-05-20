/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2013
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "matlabarray.h"

#include "arraynd.h"
#include "arrayndinfo.h"
#include "task.h"

// MATLAB header files
#include "matrix.h"


class ArrayCopier : public ParallelTask
{
};


MatlabArrayND::MatlabArrayND( const ArrayND<float>& arrnd )
    : arrnd_(arrnd)
{
    const int nrdim = arrnd.info().getNDim();
    mwSize dims[nrdim];
    for ( int idx=0; idx<nrdim; idx++ )
	dims[idx] = arrnd.info().getSize( idx );

    mxarr_ = mxCreateNumericArray( nrdim, dims, mxDOUBLE_CLASS, mxREAL );

}


MatlabArrayND::~MatlabArrayND()
{
    // do something with mxarr_
}
