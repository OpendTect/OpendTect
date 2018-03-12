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

    void		doCloseFile(Access&);
    static H5::DataType	h5DataTypeFor(ODDataType);

    static const char*	sOpenFileFirst();
    static const char*	sNeedScope();

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


#define mRetNoFile(action) \
    { pErrMsg( sOpenFileFirst() ); action; }

#define mRetNoFileInUiRv() \
    mRetNoFile( uirv.set(uiStrings::phrInternalErr(sOpenFileFirst()) ); return )

#define mRetNeedScopeInUiRv() \
{ \
    pErrMsg( sNeedScope() ); \
    uirv.set( uiStrings::phrInternalErr(sNeedScope()) ); \
    return; \
}
