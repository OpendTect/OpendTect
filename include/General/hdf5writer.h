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

    virtual void	setChunkSize(int)				= 0;

    uiRetVal		putInfo(const DataSetKey&,const IOPar&);
    uiRetVal		putData(const DataSetKey&,const ArrayNDInfo&,
				const void*,ODDataType);

protected:

    virtual void	ptInfo(const DataSetKey&,const IOPar&,uiRetVal&)= 0;
    virtual void	ptData(const DataSetKey&,const ArrayNDInfo&,
				const void*,ODDataType,uiRetVal&)	= 0;

};

} // namespace HDF5
