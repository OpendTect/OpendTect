#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"
#include "typeset.h"
#include "coord.h"
#include "uistring.h"

namespace Well
{

mExpClass(Well) DirectionalSurvey
{ mODTextTranslationClass(Well::DirectionalSurvey);
public:
    enum Method		{ MinCurv, Tangential };
			DirectionalSurvey(const Coord& surfacecrd,double kb=0);
			~DirectionalSurvey();

    void		calcTrack(const TypeSet<double>& mds,
				  const TypeSet<double>& incls,
				  const TypeSet<double>& azis,
				  TypeSet<Coord3>& track);

protected:
    Coord		surfacecoord_;
    double		kb_;
    Method		method_;
};

} // namespace Well
