#ifndef seiscommon_h
#define seiscommon_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris/Bert
 Date:		2009 / Mar 2016
 RCS:		$Id$
________________________________________________________________________

*/

#include "seistype.h"

namespace Seis
{

    mGlobal(Seis) const float	DefZEps = 1e-6f;	    //!< 1 us or 1 um
    mGlobal(Seis) const float	DefOffsetEps = 1e-3f;	    //!< 1 mm
    mGlobal(Seis) const float	DefSampleSnapDist = 1e-4f;
				//!< Default rel dist from actual sample for
				//! which value will not be interpolated. 1e-4

    mGlobal(Seis) inline bool	equalOffset( float offs1, float offs2 )
				{ return mIsEqual(offs1,offs2,DefOffsetEps); }
					//!< Undef *not* supported

} // namespace Seis


#endif
