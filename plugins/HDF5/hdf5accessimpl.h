#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5common.h"


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

			AccessImpl(const AccessImpl&)	= delete;

    Access&		acc_;
    static void		doCloseFile(Access&);

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
{ \
    const char* e_msg = "HDF5: need successful open() before setting stuff"; \
    pErrMsg(e_msg); \
    action; \
}
