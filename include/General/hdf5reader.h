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

mExpClass(General) Reader : public Access
{
public:

    virtual void		getGroups(BufferStringSet&) const	= 0;
				//!< All groups with full names (recursive)
    virtual void		getDataSets(const char*,
					    BufferStringSet&) const	= 0;
				//!< Pass a full group name
    virtual ArrayNDInfo*	getDataSizes(const DataSetKey&) const	= 0;
				//!< Can return null when:
				//!< - there is no such group
				//!< - the group has an empty dataset
    virtual ODDataType		getDataType() const			= 0;

    uiRetVal			getInfo(const DataSetKey&,IOPar&) const;
    uiRetVal			getAll(const DataSetKey&,void*) const;

    //TODO slices
    //TODO point-wise

protected:

    virtual void	gtInfo(const DataSetKey&,IOPar&,uiRetVal&) const= 0;
    virtual void	gtAll(const DataSetKey&,void*,uiRetVal&) const	= 0;

};

} // namespace HDF5
