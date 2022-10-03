#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellattribmod.h"

#include "gendefs.h"
#include "odcomplex.h"


/* !brief performs the computations needed by TWTS  !*/

namespace Well { class D2TModel; class Data; class Log; }

namespace WellTie
{

namespace GeoCalculator
{

//Well data operations
mGlobal(WellAttrib) Well::D2TModel* getModelFromVelLog(const Well::Data&,
						       const char* lognm);
mGlobal(WellAttrib) void son2TWT(Well::Log&,const Well::Data&);
mGlobal(WellAttrib) void vel2TWT(Well::Log&,const Well::Data&);
mGlobal(WellAttrib) void son2Vel(Well::Log&);
mGlobal(WellAttrib) void d2TModel2Log(const Well::D2TModel&,Well::Log&);

//others
mGlobal(WellAttrib) void removeSpikes(float* inp,int sz,int gate,int fac);
mGlobal(WellAttrib) double crossCorr(const float*,const float*,float*,int);
mGlobal(WellAttrib) void deconvolve(const float*,const float_complex*,
				    float*,int);

} // namespace GeoCalculator

} // namespace WellTie
