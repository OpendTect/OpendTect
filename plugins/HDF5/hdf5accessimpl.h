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

class ReaderImpl;
class WriterImpl;

//!brief Mixin for common stuff

mExpClass(HDF5) AccessImpl
{
public:

			AccessImpl(ReaderImpl&);
			AccessImpl(WriterImpl&);
    virtual		~AccessImpl();

    const char*		gtFileName() const;

protected:

    typedef H5::DataType H5DataType;

			AccessImpl(const AccessImpl&)	= delete;

    Access&		acc_;
    mutable H5::Group*	group_;
    mutable H5::DataSet* dataset_;

			// no throw
    void		doCloseFile(Access&);
    static H5::DataType	h5DataTypeFor(ODDataType);
    bool		atGroup(const char*&) const;
    bool		atDataSet(const char*) const;

			// can throw
    void		selectSlab(H5::DataSpace&,const SlabSpec&,
				   TypeSet<hsize_t>* pcounts=0) const;

};


mExpClass(HDF5) AccessProviderImpl : public AccessProvider
{
public:

    mDefaultFactoryInstantiation0Param( AccessProvider, AccessProviderImpl,
			"OD", toUiString("OD") );

    virtual Reader*	getReader() const;
    virtual Writer*	getWriter() const;

    static void		initHDF5(); //!< class initClass()

};

} // namespace HDF5


#define mGetDataSpaceDims( dims, nrdims, dataspace ) \
    TypeSet<hsize_t> dims( nrdims, (hsize_t)0 ); \
    dataspace.getSimpleExtentDims( dims.arr() )
