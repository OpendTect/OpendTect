#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris/Bert
 Date:		2009 / Mar 2016
 RCS:		$Id$
________________________________________________________________________

*/

#include "seismod.h"
#include "seistype.h"

namespace Seis
{

    mGlobal(Seis) inline float	cDefZEps()		{ return 1e-6f; }
				//!< 1 us or 1 um
    mGlobal(Seis) inline float	cDefOffsetEps()		{ return 1e-3f; }
				//!< 1 mm
    mGlobal(Seis) inline float	cDefSampleSnapDist()	{ return 1e-3f; }
				//!< Default rel dist from actual sample for
				//! which value will not be interpolated. 1e-4

    mGlobal(Seis) inline bool	equalOffset( float offs1, float offs2 )
				{ return mIsEqual(offs1,offs2,cDefOffsetEps());}
					//!< Undef *not* supported

    mGlobal(Seis) uiString sSEGPositive();
    mGlobal(Seis) uiString sSEGNegative();

} // namespace Seis


