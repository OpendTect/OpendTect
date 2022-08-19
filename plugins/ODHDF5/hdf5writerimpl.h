#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "hdf5common.h"
#include "hdf5accessimpl.h"
#include "hdf5writer.h"


namespace HDF5
{

mExpClass(ODHDF5) WriterImpl : public Writer
			     , public AccessImpl
{ mODTextTranslationClass(Writer)
public:

			WriterImpl();
    virtual		~WriterImpl();

private:

    Reader*		createCoupledReader() const override;

    void		setCompressionLevel( unsigned lvl ) override
			{ compressionlvl_ = lvl; }

    const char*		fileName() const override	{ return gtFileName(); }
    void		openFile(const char*,uiRetVal&,bool) override;
    void		closeFile() override		{ doCloseFile(*this); }

    DataSetKey		scope() const override		{ return gtScope(); }
    od_int64		curGroupID() const override	{ return gtGroupID(); }
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

    H5::Group*		ensureGroup(const char*,uiRetVal&) override;
    H5::DataSet*	crDS(const DataSetKey&,const ArrayNDInfo&,ODDataType,
			     uiRetVal&) override;
    H5::DataSet*	crTxtDS(const DataSetKey&,uiRetVal&) override;
    void		reSzDS(const ArrayNDInfo&,H5::DataSet&,
			       uiRetVal&) override;

    void		ptSlab(const SlabSpec&,const void*,H5::DataSet&,
			       uiRetVal&) override;
    void		ptAll(const void*,H5::DataSet&,uiRetVal&) override;
    void		ptStrings(const BufferStringSet&,H5::Group&,
				  H5::DataSet*,const char* dsnm,
				  uiRetVal&) override;

    void		setAttribute(const char* ky,const char* val,
				     const DataSetKey* =nullptr) override;
    void		setAttribute(const char* ky,const char* val,
				     H5::H5Object&);
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
    void		rmAttrib(const char*,H5::H5Object&) override;
    void		rmAllAttribs(H5::H5Object&) override;
    void		ptInfo(const IOPar&,H5::H5Object&,uiRetVal&) override;

    bool		rmObj(const DataSetKey&) override;

    unsigned		compressionlvl_ = 1;

};

} // namespace HDF5
