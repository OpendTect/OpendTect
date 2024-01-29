#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "hdf5access.h"
#include "ranges.h"


namespace OD {
    namespace JSON {
	class ValueSet;
    };
};

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

    virtual void	getGroups(BufferStringSet&) const		= 0;
				//!< All groups with full names (recursive)
    virtual void	getDataSets(const char* fullgrpnm,
				    BufferStringSet&) const		= 0;
				//!< Pass a full group name

    ODDataType		getDataType(const DataSetKey&,uiRetVal&) const;
    ArrayNDInfo*	getDataSizes(const DataSetKey&,uiRetVal&) const;
    nr_dims_type	getNrDims(const DataSetKey&,uiRetVal&) const;
    nr_dims_type	nrDims() const		{ return gtNrDims(); }
    size_type		dimSize(const DataSetKey&,dim_idx_type,uiRetVal&) const;

    uiRetVal		getSlab(const DataSetKey&,const SlabSpec&,void*) const;
    uiRetVal		getAll(const DataSetKey&,void*) const;
				//!< Get the entire data set in current scope
    template <class T>
    uiRetVal		get(const DataSetKey&,TypeSet<T>&) const;
    uiRetVal		get(const DataSetKey&,BufferStringSet&) const;
    uiRetVal		getValue(const DataSetKey&,NDPos,void*) const;
				//!< Get a single point value
    uiRetVal		getValues(const DataSetKey&,const NDPosBufSet&,
				  void*) const;
				//!< Get a set of distinct points' values

    uiRetVal		getComment(const DataSetKey&,BufferString&) const;
			// null = root scope
    virtual bool	hasAttribute(const char*,
				     const DataSetKey* =nullptr) const	= 0;
    virtual int		getNrAttributes(const DataSetKey* = nullptr ) const = 0;
    uiRetVal		getAttributeNames(BufferStringSet&,
				    const DataSetKey* = nullptr) const;
    virtual bool	getAttribute(const char*,BufferString&,
				     const DataSetKey* =nullptr) const	= 0;
#define mHDF5DeclFns(type) \
    virtual bool	getAttribute(const char*,type&, \
				     const DataSetKey* =nullptr) const	= 0;
			mHDF5DeclFns(od_int16);
			mHDF5DeclFns(od_uint16);
			mHDF5DeclFns(od_int32);
			mHDF5DeclFns(od_uint32);
			mHDF5DeclFns(od_int64);
			mHDF5DeclFns(od_uint64);
			mHDF5DeclFns(float);
			mHDF5DeclFns(double);
#undef mHDF5DeclFns
    uiRetVal		get(IOPar&,const DataSetKey* =nullptr) const;
    virtual uiRetVal	readJSonAttribute(const char*,
					  OD::JSON::ValueSet&,
					  const DataSetKey* =nullptr) const = 0;

    uiRetVal		getVersion(const DataSetKey&,unsigned&) const;

    bool		isReader() const override		{ return true; }

protected:

    static uiString	sBadDataSpace()
			{ return sHDF5Err(tr("Unexpected DataSet found")); }

private:

    virtual ODDataType	gtDataType(const H5::DataSet&) const	= 0;
    virtual ArrayNDInfo* gtDataSizes(const H5::DataSet&) const		= 0;
    virtual nr_dims_type gtNrDims() const				= 0;
    size_type		dimSize(const H5::DataSet&,dim_idx_type) const;

    virtual void	gtSlab(const H5::DataSet&,const SlabSpec&,void*,
			       uiRetVal&) const = 0;
    virtual void	gtAll(const H5::DataSet&,void*,uiRetVal&) const = 0;
    virtual void	gtStrings(const H5::DataSet&,BufferStringSet&,
				  uiRetVal&) const			= 0;
    virtual void	gtValues(const H5::DataSet&,const NDPosBufSet&,void*,
				 uiRetVal&) const			= 0;

    virtual void	gtComment(const H5::H5Location&,const char* name,
				  BufferString&,uiRetVal&) const = 0;
    virtual void	gtAttribNames(const H5::H5Object&,
				BufferStringSet&) const = 0;

    virtual void	gtInfo(const H5::H5Object&,IOPar&,
				uiRetVal&) const			= 0;
    virtual unsigned	gtVersion(const H5::H5Object&,uiRetVal&) const	= 0;

};


template <class T>
inline uiRetVal Reader::get( const DataSetKey& dsky, TypeSet<T>& vals ) const
{
    uiRetVal uirv;
    if ( getNrDims(dsky,uirv) != 1 )
	{ pErrMsg("Only read TypeSet from 1-D dataset"); return uirv; }
    if ( getDataType(dsky,uirv) != OD::GetDataRepType<T>() )
	{ pErrMsg("Wrong type"); return uirv; }

    const size_type sz = dimSize( dsky, 0, uirv );
    if ( sz < 1 )
	return uirv;

    vals.setSize( sz );
    uirv = getAll( dsky, vals.arr() );
    return uirv;
}

} // namespace HDF5
