#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "hdf5accessimpl.h"
#include "hdf5writer.h"

namespace HDF5
{

mExpClass(ODHDF5) WriterImpl : public Writer
			     , public AccessImpl
{ mODTextTranslationClass(Writer)
public:

			WriterImpl();
			~WriterImpl();

private:

    Reader*		createCoupledReader() const override;

    void		setCompressionLevel( unsigned lvl ) override
			{ compressionlvl_ = lvl; }

    const char*		fileName() const override
					{ return AccessImpl::gtFileName(); }
    uiRetVal		open(const char* fnm);
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

    GroupID		ensureGroup(const char*,uiRetVal&) override;
    DatasetID		crDS(const DataSetKey&,const ArrayNDInfo&,ODDataType,
			     uiRetVal&) override;
    DatasetID		crTxtDS(const DataSetKey&,uiRetVal&) override;
    void		reSzDS(const ArrayNDInfo&,const DatasetID&,
			       uiRetVal&) override;

    void		ptSlab(const SlabSpec&,const void*,const DatasetID&,
			       uiRetVal&) override;
    void		ptAll(const void*,const DatasetID&,uiRetVal&) override;
    void		ptStrings(const BufferStringSet&,const GroupID&,
				  const DatasetID&,const char* dsnm,
				  uiRetVal&) override;

    void		stComment(const LocationID& ,const char* name,
				  const char* comment,uiRetVal&) override;
    void		setAttribute(const char* ky,const char* val,
				     const DataSetKey* =nullptr) override;
    void		setAttribute(const char* ky,const char* val,
				     const ObjectID&);
#define mHDF5DeclFns(fnnm,type) \
    void		fnnm##Attribute(const char*,type, \
					const DataSetKey* =nullptr) override;
			mHDF5DeclFns(set,od_int16);
			mHDF5DeclFns(set,od_uint16);
			mHDF5DeclFns(set,od_int32);
			mHDF5DeclFns(set,od_uint32);
			mHDF5DeclFns(set,od_int64);
			mHDF5DeclFns(set,od_uint64);
			mHDF5DeclFns(set,float);
			mHDF5DeclFns(set,double);
#undef mHDF5DeclFns
    void		rmAttrib(const char*,const ObjectID&) override;
    void		rmAllAttribs(const ObjectID&) override;
    void		ptInfo(const IOPar&,const ObjectID&,uiRetVal&) override;
    uiRetVal		writeJSonAttribute(const char*,
				    const OD::JSON::ValueSet&,
				    const DataSetKey* =nullptr) override;

    bool		rmObj(const DataSetKey&) override;
    void		renObj(const LocationID&,const char* from,
			       const char* to,uiRetVal&) override;

    unsigned		compressionlvl_ = 1;

};

} // namespace HDF5
