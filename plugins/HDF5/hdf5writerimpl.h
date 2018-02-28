#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5common.h"
#include "H5Cpp.h"


namespace HDF5
{

mExpClass(HDF5) WriterImpl : public Writer
{
public:

			    WriterImpl();
			    ~WriterImpl();

    virtual uiRetVal	    open(const char*);

protected:

    H5::H5File*		    file_;

};

} // namespace HDF5
