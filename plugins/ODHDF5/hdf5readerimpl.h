#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "hdf5accessimpl.h"
#include "hdf5reader.h"

namespace HDF5
{

mExpClass(ODHDF5) ReaderImpl : public Reader
			     , public AccessImpl
{ mODTextTranslationClass(ReaderImpl)
public:

			ReaderImpl();
			ReaderImpl(const FileID&);
    virtual		~ReaderImpl();

private:

    const char*		fileName() const override
					{ return AccessImpl::gtFileName(); }
    void		openFile(const char*,uiRetVal&,bool) override;
    void		closeFile() override		{ doCloseFile(*this); }

    DataSetKey		scope() const override
				{ return AccessImpl::gtScope(); }
    od_int64		curGroupID() const override
				{ return AccessImpl::gtGroupID(); }
    LocationID		setLocation( const DataSetKey* dsky ) override
				{ return AccessImpl::stLocation( dsky ); }
    LocationID		getLocation( const DataSetKey* dsky ) const override
				{ return AccessImpl::stLocation( dsky ); }
    ObjectID		setScope( const DataSetKey* dsky ) override
				{ return AccessImpl::stScope( dsky ); }
    ObjectID		getScope( const DataSetKey* dsky ) const override
				{ return AccessImpl::stScope( dsky ); }
    GroupID		setGrpScope( const DataSetKey* dsky ) override
				{ return AccessImpl::stGrpScope( dsky ); }
    GroupID		getGrpScope( const DataSetKey* dsky ) const override
				{ return AccessImpl::stGrpScope( dsky ); }
    DatasetID		setDSScope( const DataSetKey& dsky ) override
				{ return AccessImpl::stDSScope( dsky ); }
    DatasetID		getDSScope( const DataSetKey& dsky ) const override
				{ return AccessImpl::stDSScope( dsky ); }


    void		getGroups(BufferStringSet&) const override;
    void		getSubGroups(const char* grpnm,
				     BufferStringSet&) const override;
    void		getDataSets(const char* grpnm,
				    BufferStringSet&) const override;
    void		gtComment(const LocationID&,const char* name,
				  BufferString&,uiRetVal&) const override;
    unsigned		gtVersion(const ObjectID&,uiRetVal&) const override;

    void		listObjs(const GroupID&,BufferStringSet&,
				 bool wantgroups) const;
    DatatypeID		h5DataType(const DatasetID&) const;
    ODDataType		gtDataType(const DatasetID&) const override;
    ArrayNDInfo*	gtDataSizes(const DatasetID&) const override;
    nr_dims_type	gtNrDims() const override	{ return nrdims_; }

    void		gtSlab(const DatasetID&,const SlabSpec&,void*,
			       uiRetVal&) const override;
    void		gtAll(const DatasetID&,void*,
			      uiRetVal&) const override;
    void		gtStrings(const DatasetID&,BufferStringSet&,
				  uiRetVal&) const override;
    void		gtValues(const DatasetID&,const NDPosBufSet&,
				 void*,uiRetVal&) const override;

    bool		hasAttribute(const char*,
				     const DataSetKey* =nullptr) const override;
    int			getNrAttributes(
				    const DataSetKey* =nullptr) const override;
    void		gtAttribNames(const ObjectID&,
				BufferStringSet&) const override;

    bool		getAttribute(const char*,BufferString&,
				     const DataSetKey* =nullptr) const override;
#define mHDF5DeclFns(type) \
    bool		getAttribute(const char*,type&, \
				     const DataSetKey* =nullptr) const override;
			mHDF5DeclFns(od_int16);
			mHDF5DeclFns(od_uint16);
			mHDF5DeclFns(od_int32);
			mHDF5DeclFns(od_uint32);
			mHDF5DeclFns(od_int64);
			mHDF5DeclFns(od_uint64);
			mHDF5DeclFns(float);
			mHDF5DeclFns(double);
#undef mHDF5DeclFns
    void		gtInfo(const ObjectID&,IOPar&,
			       uiRetVal&) const override;
    uiRetVal		readJSonAttribute(const char*,OD::JSON::ValueSet&,
				  const DataSetKey* =nullptr) const override;

};

} // namespace HDF5
