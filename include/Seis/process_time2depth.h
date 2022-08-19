#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

//!Keys for od_process_time2depth.cc

struct ProcessTime2Depth
{
    static const char*	sKeyInputVolume()	{ return "Input volume"; }
    static const char*	sKeyOutputVolume()	{ return "Output volume"; }
    static const char*	sKeyZTransPar()		{ return "ZTrans"; }
    static const char*	sKeyIsTimeToDepth()	{ return "Time to depth"; }
};
