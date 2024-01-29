#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "hdf5common.h"
#include "hdf5accessimpl.h"
#include "hdf5reader.h"


namespace HDF5
{

mExpClass(ODHDF5) ReaderImpl : public Reader
			     , public AccessImpl
{ mODTextTranslationClass(ReaderImpl)
public:

			ReaderImpl();
			ReaderImpl(const H5::H5File&);
    virtual		~ReaderImpl();

private:

    const char*		fileName() const override { return gtFileName(); }
    void		openFile(const char*,uiRetVal&,bool) override;
    void		closeFile() override		{ doCloseFile(*this); }

    DataSetKey		scope() const override		{ return gtScope(); }
    od_int64		curGroupID() const override	{ return gtGroupID(); }
    H5::H5Location*	setLocation( const DataSetKey* dsky ) override
						{ return stLocation( dsky ); }
    H5::H5Location*	getLocation( const DataSetKey* dsky ) const override
						{ return stLocation( dsky ); }
    H5::H5Object*	setScope( const DataSetKey* dsky ) override
						{ return stScope( dsky ); }
    H5::H5Object*	getScope( const DataSetKey* dsky ) const override
						{ return stScope( dsky ); }
    H5::Group*		setGrpScope( const DataSetKey* dsky ) override
						{ return stGrpScope( dsky ); }
    H5::Group*		getGrpScope( const DataSetKey* dsky ) const override
						{ return stGrpScope( dsky ); }
    H5::DataSet*	setDSScope( const DataSetKey& dsky ) override
						{ return stDSScope( dsky ); }
    H5::DataSet*	getDSScope( const DataSetKey& dsky ) const override
						{ return stDSScope( dsky ); }

    void		getGroups(BufferStringSet&) const override;
    void		getDataSets(const char* grpnm,
				    BufferStringSet&) const override;
    void		gtComment(const H5::H5Location&,const char* name,
				  BufferString&,uiRetVal&) const override;
    unsigned		gtVersion(const H5::H5Object&,uiRetVal&) const override;
    template <class H5Dir>
    void		listObjs(const H5Dir&,BufferStringSet&,
				 bool wantgroups) const;

    const H5DataType&	h5DataType(const H5::DataSet&) const;
    ODDataType		gtDataType(const H5::DataSet&) const override;
    ArrayNDInfo*	gtDataSizes(const H5::DataSet&) const override;
    nr_dims_type	gtNrDims() const override	{ return nrdims_; }

    void		gtSlab(const H5::DataSet&,const SlabSpec&,void*,
			       uiRetVal&) const override;
    void		gtAll(const H5::DataSet&,void*,
			      uiRetVal&) const override;
    void		gtStrings(const H5::DataSet&,BufferStringSet&,
				  uiRetVal&) const override;
    void		gtValues(const H5::DataSet&,const NDPosBufSet&,
				 void*,uiRetVal&) const override;

    bool		hasAttribute(const char*,
				     const DataSetKey* =nullptr) const override;
    int			getNrAttributes(
				    const DataSetKey* =nullptr) const override;
    void		gtAttribNames(const H5::H5Object&,
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
    void		gtInfo(const H5::H5Object&,IOPar&,
			       uiRetVal&) const override;
    uiRetVal		readJSonAttribute(const char*,OD::JSON::ValueSet&,
				  const DataSetKey* =nullptr) const override;

};

} // namespace HDF5
