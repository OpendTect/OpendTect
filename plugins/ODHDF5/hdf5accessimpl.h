#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odhdf5mod.h"

#include "hdf5access.h"
#include "threadlock.h"
#include "typeset.h"

#include <hdf5.h>


namespace HDF5
{

class ReaderImpl;
class WriterImpl;

//!brief Mixin for common stuff

mExpClass(ODHDF5) AccessImpl
{
public:

			AccessImpl(ReaderImpl&);
			AccessImpl(WriterImpl&);
    virtual		~AccessImpl();

    static bool		isParallelEnabled();
    static void		setErrPrint(bool);	//!< user switch on/off

protected:

    const char*		gtFileName() const;
    DataSetKey		gtScope() const;
    od_int64		gtGroupID() const;

    bool		haveGroup() const;
    bool		haveDataSet() const;
    bool		atGroup(const char*&) const;
    bool		atDataSet(const char*) const;

			// no throw
    void		doCloseFile(Access&);

    GroupID		selectGroup(const char*) const;
    DatasetID		selectDataSet(const char*) const;
    LocationID		stLocation(const DataSetKey*) const;
    LocationID		stLocation(const DataSetKey*);
    ObjectID		stScope(const DataSetKey*) const;
    ObjectID		stScope(const DataSetKey*);
    GroupID		stGrpScope(const DataSetKey*) const;
    GroupID		stGrpScope(const DataSetKey*);
    DatasetID		stDSScope(const DataSetKey&) const;
    DatasetID		stDSScope(const DataSetKey&);

    mutable		Threads::Lock	lock_;
    void		selectSlab(const DataspaceID&,const SlabSpec&,
				   TypeSet<hsize_t>* pcounts=0) const;
				//!< can throw, use in try block
    static bool		haveErrPrint();

    static DatatypeID h5DataTypeFor(ODDataType);

    Access&		acc_;
    mutable GroupID	group_;
    mutable TypeSet<hid_t> previousgroupids_;
    mutable DatasetID	dataset_;
    mutable ArrayNDInfo::nr_dims_type nrdims_ = -1;

private:
			mOD_DisableCopy(AccessImpl);

    static bool		validH5Obj(const ObjectID&);

    static void		disableErrPrint(); // before action with 'normal' throw

};


mExpClass(ODHDF5) AccessProviderImpl : public AccessProvider
{
public:

    mDefaultFactoryInstantiation( AccessProvider, AccessProviderImpl,
			"OD", toUiString("OD") );

private:

    Reader*		getReader() const override;
    Writer*		getWriter() const override;

public:

    static void		initHDF5(); //!< class initClass()

};

} // namespace HDF5


#define mGetDataSpaceDims( dims, nrdims, dataspace ) \
    TypeSet<hsize_t> dims( nrdims, (hsize_t)0 ); \
    H5Sget_simple_extent_dims( dataspace, dims.arr(), nullptr )
