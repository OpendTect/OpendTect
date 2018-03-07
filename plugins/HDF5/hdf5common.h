#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5mod.h"
#include "hdf5access.h"

namespace HDF5
{

} // namespace HDF5


#define mCatchAnyNoMsg(act) \
    catch ( ... ) \
	{ act; }

#define mCatchNonHDF(act) \
    catch ( std::exception& exc ) \
	{ const char* exc_msg = exc.what(); act; } \
    catch ( ... ) \
	{ const char* exc_msg = "Unexpected non-std exception"; act; }

#define mCatchUnexpected(act) \
    catch ( H5::Exception& exc ) \
	{ const char* exc_msg = exc.getCDetailMsg(); pErrMsg(exc_msg); act; } \
    catch ( std::exception& exc ) \
	{ const char* exc_msg = exc.what(); pErrMsg(exc_msg); act; } \
    catch ( ... ) \
	{ const char* exc_msg = "Unexpected non-std exception"; \
	    pErrMsg(exc_msg); act; }
