#pragma once

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

class Info;
class Data;
class D2TModel;
class Marker;
class MarkerSet;
class Log;
class LogSet;
class Track;

#define mWellNrSubObjTypes 9

enum SubObjType		{ Inf=0, Trck=1, D2T=2, CSMdl=3, Mrkrs=4, Logs=5,
			   LogInfos=6, DispProps2D=7, DispProps3D=8 };
mGlobal(Well) int	nrSubObjTypes(); //    { return mWellNrSubObjTypes; }

mGlobal(Well) float	getDefaultVelocity();
			//!< If survey unit is depth-feet, it returns the
			//!< equivalent of 8000 (ft/s), otherwise 2000 (m/s).
			//!< Its purpose is to get nice values of velocity
			//!< where we may need such functionality
			//!< (eg. replacement velocity for wells).

} // namespace Well
