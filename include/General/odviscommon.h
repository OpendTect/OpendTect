#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2014
________________________________________________________________________

-*/

#include "generalmod.h"
#include "uistring.h"


namespace OD
{

// Surface stuff

enum class SurfaceResolution : int
{
    Automatic = 0,
    Full = 1,
    Half = 2,
    Quarter = 3,
    OneEighth = 4,
    OneSixteenth = 5,
    OneThirtySecond = 6,

};

inline SurfaceResolution cMinSurfaceResolution()
{ return SurfaceResolution::OneThirtySecond; }
inline const char* sSurfaceResolutionSettingsKey()
{ return "dTect.Horizon.Resolution"; }

mGlobal(General) SurfaceResolution getDefaultSurfaceResolution();
mGlobal(General) uiWord getSurfaceResolutionDispStr(SurfaceResolution);
mGlobal(General) const char* defSurfaceDataColSeqName();

} // namespace OD
