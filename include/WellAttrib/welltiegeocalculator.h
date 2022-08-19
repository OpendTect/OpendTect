#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gendefs.h"
#include "odcomplex.h"
#include "wellattribmod.h"

/* !brief performs the computations needed by TWTS  !*/

namespace Well { class D2TModel; class Log; class Data; }

namespace WellTie
{

mExpClass(WellAttrib) GeoCalculator
{
public :
//Well data operations
    Well::D2TModel*	getModelFromVelLog(const Well::Data&,
					   const char* lognm) const;

    void		son2TWT(Well::Log&,const Well::Data&) const;
    void		vel2TWT(Well::Log&,const Well::Data&) const;
    void		son2Vel(Well::Log&) const;
    void		d2TModel2Log(const Well::D2TModel&,Well::Log&) const;

//others
    void		removeSpikes(float* inp,int sz,int gate,int fac) const;
    double		crossCorr(const float*,const float*,float*,int) const;
    void		deconvolve(const float*,const float_complex*,
				   float*,int) const;
};

} // namespace WellTie
