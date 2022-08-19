#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "seistype.h"

namespace Seis
{

    mGlobal(Seis) inline float	cDefZEps()		{ return 1e-6f; }
				//!< 1 us or 1 um
    mGlobal(Seis) inline float	cDefOffsetEps()		{ return 1e-3f; }
				//!< 1 mm
    mGlobal(Seis) inline float	cDefSampleSnapDist()	{ return 1e-4f; }
				//!< Default rel dist from actual sample for
				//! which value will not be interpolated. 1e-4

    mGlobal(Seis) inline bool	equalOffset( float offs1, float offs2 )
				{ return mIsEqual(offs1,offs2,cDefOffsetEps());}
					//!< Undef *not* supported


    mGlobal(Seis) inline uiString sSEGPositive()
		    { return od_static_tr("sSEGPositive",
					"SEG Positive"); }
    mGlobal(Seis) inline uiString sSEGNegative()
		    { return od_static_tr("sSEGNegative",
			    "SEG Negative (North Sea)"); }

} // namespace Seis
