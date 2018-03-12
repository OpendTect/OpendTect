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

/*!\brief writes to HDF5 file

Notes:
* You can only write info *after* you have written the dataset itself. This
  may be annoying, but it saves us the hassle/management of dummy datasets.
* Writing datasets in the root group is not recommended, but may work.
* You can write info for an entire group by simpy leaving the dataset name
  empty in the DataSetKey.

  */

mExpClass(General) Writer : public Access
{
public:

    virtual void	setChunkSize(int)				= 0;

    uiRetVal		putData(const DataSetKey&,const ArrayNDInfo&,
				const void*,ODDataType);
    uiRetVal		putInfo(const DataSetKey&,const IOPar&);

protected:

    virtual void	ptInfo(const DataSetKey&,const IOPar&,uiRetVal&)= 0;
    virtual void	ptData(const DataSetKey&,const ArrayNDInfo&,
				const void*,ODDataType,uiRetVal&)	= 0;

};

} // namespace HDF5
