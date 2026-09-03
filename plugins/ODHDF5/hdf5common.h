#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odhdf5mod.h"

#include "bufstring.h"

namespace HDF5
{

BufferString	hdf5ErrMsg();

} // namespace HDF5


#define mCatchHDF( id, act ) \
    do { \
	if ( (id) == H5I_INVALID_HID ) \
	{ \
	    const BufferString hdfmsg = HDF5::hdf5ErrMsg(); \
	    const char* mUnusedVar exc_msg = hdfmsg.buf(); \
	    act; \
	} \
    } while ( 0 )

#define mAddHDFErr2uiRv( msg ) \
    uirv.add( (msg).addMoreInfo( sHDF5Err( toUiString(exc_msg) ) ) )

#define mCatchHDFAdd2uiRv( id, msg ) \
    mCatchHDF( id, mAddHDFErr2uiRv(msg); return )


#define mCatchAnyNoMsg( act ) \
    catch ( ... ) \
	{ act; }

#define mCatchUnexpected( act ) \
    catch ( std::exception& exc ) \
	{ const char* mUnusedVar exc_msg = exc.what(); pErrMsg(exc_msg); act;} \
    catch ( ... ) \
	{ const char* mUnusedVar exc_msg = "Unexpected non-std exception"; \
	    pErrMsg(exc_msg); act; }
