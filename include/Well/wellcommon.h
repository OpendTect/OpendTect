#ifndef wellcommon_h
#define wellcommon_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "wellmod.h"
#include "gendefs.h"

namespace Well
{

class D2TModel;
class Data;
class Info;
class Marker;
class MarkerSet;
class Log;
class LogSet;
class Track;

mGlobal(Well) float	getDefaultVelocity();
			//!< If survey unit is depth-feet, it returns the
			//!< equivalent of 8000 (ft/s), otherwise 2000 (m/s).
			//!< Its purpose is to get nice values of velocity
			//!< where we may need such functionality
			//!< (eg. replacement velocity for wells).

} // namespace Well

#endif
