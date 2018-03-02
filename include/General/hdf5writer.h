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

    virtual void	setDataType(OD::FPDataRepType)			= 0;
    virtual void	setChunkSize(int)				= 0;

    virtual uiRetVal	putInfo(const GroupPath&,const IOPar&)		= 0;
    virtual uiRetVal	putData(const GroupPath&,const ArrayND<float>&,
				const IOPar* info=0)			= 0;

};

} // namespace HDF5
