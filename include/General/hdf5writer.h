#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          March 2018
________________________________________________________________________

-*/

#include "hdf5access.h"


namespace HDF5
{

mExpClass(General) Writer : public Access
{
public:

    virtual void	setDims(const ArrayNDInfo&)		= 0;
    virtual void	setChunkSize(int)			= 0;

};

} // namespace HDF5
