#ifndef hdf5reader_h
#define hdf5reader_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		March 2018
________________________________________________________________________

-*/

#include "hdf5access.h"
#include "ranges.h"


namespace HDF5
{

/*!\brief Reads HDF5 file data.

  You can only get actual data after you set the scope, i.e. you have to
  select the 'current' DataSet using the DataSetKey. Leave the
  dataset name empty only for retrieving attributes for the entire group.

  */

mExpClass(General) Reader : public Access
{ mODTextTranslationClass(HDF5::Reader)
public:

    typedef TypeSet<NDPosBuf>	NDPosBufSet;

    virtual void	getGroups(BufferStringSet&) const	= 0;
				//!< All groups with full names (recursive)
    virtual void	getDataSets(const char*,
				    BufferStringSet&) const	= 0;
				//!< Pass a full group name

    // use setScope() before reading anything using the functions below

    nr_dims_type	nrDims() const	{ return gtNrDims(); }
    size_type		dimSize(dim_idx_type) const;
    ArrayNDInfo*	getDataSizes() const { return gtDataSizes(); }

    virtual ODDataType	getDataType() const			= 0;
    uiRetVal		getInfo(IOPar&) const;

    template <class T>
    uiRetVal		get(TypeSet<T>&) const;
    uiRetVal		get(BufferStringSet&) const;
    uiRetVal		getAll(void*) const;
				//!< Get the entire data set in current scope
    uiRetVal		getPoint(NDPos,void*) const;
				//!< Get a single point value
    uiRetVal		getPoints(const NDPosBufSet&,void*) const;
				//!< Get a set of distinct points' values
    uiRetVal		getSlab(const SlabSpec&,void*) const;

    virtual bool	isReader() const		{ return true; }

protected:

    virtual void	gtInfo(IOPar&,uiRetVal&) const			= 0;
    virtual void	gtStrings(BufferStringSet&,uiRetVal&) const	= 0;
    virtual void	gtAll(void*,uiRetVal&) const			= 0;
    virtual void	gtPoints(const NDPosBufSet&,void*,
				 uiRetVal&) const			= 0;
    virtual void	gtSlab(const SlabSpec&,void*,uiRetVal&) const	= 0;

    virtual ArrayNDInfo* gtDataSizes() const				= 0;
    virtual nr_dims_type gtNrDims() const				= 0;
			//!< in the current scope

    static uiString	sBadDataSpace()
			{ return sHDF5Err(tr("Unexpected DataSet found")); }

};


template <class T>
inline uiRetVal Reader::get( TypeSet<T>& vals ) const
{
    uiRetVal uirv;
    if ( nrDims() != 1 )
	{ pErrMsg("Only read TypeSet from 1-D dataset"); return uirv; }
    if ( getDataType() != OD::GetDataRepType<T>() )
	{ pErrMsg("Wrong type"); return uirv; }

    const size_type sz = dimSize( 0 );
    if ( sz < 1 )
	return uirv;

    vals.setSize( sz );
    gtAll( vals.arr(), uirv );
    return uirv;
}

} // namespace HDF5

#endif
