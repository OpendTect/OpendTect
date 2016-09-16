#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2009
________________________________________________________________________

-*/


#include "seiscommon.h"

//!Keys for od_process_time2depth.cc

struct ProcessTime2Depth
{
    static const char*	sKeyInputVolume()	{ return "Input volume"; }
    static const char*	sKeyOutputVolume()	{ return "Output volume"; }
    static const char*	sKeyZTransPar()		{ return "ZTrans"; }
    static const char*	sKeyIsTimeToDepth()	{ return "Time to depth"; }
};
