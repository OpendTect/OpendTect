#ifndef directionalsurvey_h
#define directionalsurvey_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Farrukh Qayyum
 Date:		Aug 2017
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellmod.h"
#include "typeset.h"
#include "coord.h"

namespace Well
{

class DirectionalSurvey
{
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

#endif
