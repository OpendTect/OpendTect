#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odhdf5mod.h"
#include "hdf5access.h"

namespace HDF5
{

} // namespace HDF5


// Catch stuff. Helpers first:

#define mCatchHDF( act ) \
    catch ( H5::Exception& exc ) \
	{ const char* mUnusedVar exc_msg = exc.getCDetailMsg(); act; }

#define mCatchHDFAdd2uiRv() \
    mCatchHDF( uirv.add( sHDF5Err( toUiString(exc_msg) ) ) )

#define mCatchNonHDF( act ) \
    catch ( std::exception& exc ) \
	{ const char* mUnusedVar exc_msg = exc.what(); act; } \
    catch ( ... ) \
	{ const char* mUnusedVar exc_msg = "Unexpected non-std exception"; act; }

#define mCatchNonHDFAdd2uiRv( err ) \
    mCatchNonHDF( uirv.add(err.addMoreInfo(toUiString(exc_msg))) )


// Catch stuff. To use:

#define mCatchAnyNoMsg( act ) \
    catch ( ... ) \
	{ act; }

#define mCatchAdd2uiRv( msg ) \
    mCatchHDFAdd2uiRv() \
    mCatchNonHDFAdd2uiRv( msg )

#define mCatchUnexpected( act ) \
    catch ( H5::Exception& exc ) \
	{ const char* mUnusedVar exc_msg = exc.getCDetailMsg(); pErrMsg(exc_msg); act; } \
    catch ( std::exception& exc ) \
	{ const char* mUnusedVar exc_msg = exc.what(); pErrMsg(exc_msg); act; } \
    catch ( ... ) \
	{ const char* mUnusedVar exc_msg = "Unexpected non-std exception"; \
	    pErrMsg(exc_msg); act; }

// Err Ret stuff

#define mRetNoFile(action) \
    { pErrMsg( sOpenFileFirst() ); action; }

#define mRetNeedScopeInUiRv() \
    mPutInternalInUiRv( uirv, sNeedScope(), return )
