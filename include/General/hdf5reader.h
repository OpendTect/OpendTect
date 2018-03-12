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

/*!\brief Reads HDF5 file data.

  You can only get actual data after you set the scope, i.e. you have to
  select the 'current' DataSet using the DataSetKey. Leave the
  dataset name empty only for retrieving attributes for the entire group.

  */

mExpClass(General) Reader : public Access
{
public:

    virtual void		getGroups(BufferStringSet&) const	= 0;
				//!< All groups with full names (recursive)
    virtual void		getDataSets(const char*,
					    BufferStringSet&) const	= 0;
				//!< Pass a full group name

    virtual bool		setScope(const DataSetKey&)		= 0;
				//!< Must be done before any data query

    virtual ArrayNDInfo*	getDataSizes() const			= 0;
				//!< Can return null when:
				//!< - there is no such group
				//!< - the group has an empty dataset
    virtual ODDataType		getDataType() const			= 0;

    uiRetVal			getInfo(IOPar&) const;
    uiRetVal			getAll(void*) const;
				//!< Get the entire data set in current scope

    //TODO slices
    //TODO point-wise

protected:

    virtual void	gtInfo(IOPar&,uiRetVal&) const= 0;
    virtual void	gtAll(void*,uiRetVal&) const	= 0;

};

} // namespace HDF5
