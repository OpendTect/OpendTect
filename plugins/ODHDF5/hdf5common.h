#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odhdf5mod.h"

namespace HDF5
{

} // namespace HDF5


// Catch stuff. Helpers first:

#define mCatchHDF(id, act) \
    do { \
	if ((id) < 0) { \
	    const char* exc_msg = nullptr; \
	    H5Eget_auto2(H5E_DEFAULT, &my_error_handler, (void**)&exc_msg); \
	    act; \
	    if (exc_msg) free((void*)exc_msg); \
	} \
    } while (0)

    #define mCatchHDFAdd2uiRv() \
    mCatchHDF( uirv.add( sHDF5Err( toUiString(exc_msg) ) ) )

#define mCatchNonHDF( act ) \
    catch ( std::exception& exc ) \
       { const char* mUnusedVar exc_msg = exc.what(); act; } \
    catch ( ... ) \
       { const char* mUnusedVar exc_msg = "Unexpected non-std exception"; \
	 act; }

#define mCatchNonHDFAdd2uiRv( err ) \
    mCatchNonHDF( uirv.add(err.addMoreInfo(toUiString(exc_msg))) )


// Catch stuff. To use:

#define mCatchAnyNoMsg( act ) \
    catch ( ... ) \
	{ act; }

#define mCatchAdd2uiRv(msg) \
    do { \
	mCatchHDFAdd2uiRv(newfile); \
	mCatchNonHDFAdd2uiRv(msg); \
    } while (0)

#define mCatchUnexpected( act ) \
    catch ( H5::Exception& exc ) \
	{ const char* mUnusedVar exc_msg = exc.getCDetailMsg(); \
	  ErrMsg(exc_msg); act; } \
    catch ( std::exception& exc ) \
	{ const char* mUnusedVar exc_msg = exc.what(); pErrMsg(exc_msg); \
	  act; } \
    catch ( ... ) \
	{ const char* mUnusedVar exc_msg = "Unexpected non-std exception"; \
	    pErrMsg(exc_msg); act; }

// Err Ret stuff

#define mRetNoFile(action) \
    { pErrMsg( sOpenFileFirst() ); action; }

#define mRetNeedScopeInUiRv() \
    mPutInternalInUiRv( uirv, sNeedScope(), return )
