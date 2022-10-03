#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "emposid.h"
#include "namedobj.h"
#include "position.h"

namespace Well { class Track; class D2TModel; }
namespace EM { class Horizon2D; class Horizon3D; }

/*!
\brief Used to give well/horizon intersection. In theory more than one
intersection is possible ( in case of faults or deviated tracks along the
horizon ) but only one pos will be returned.
*/

mExpClass(WellAttrib) WellHorIntersectFinder
{
public:
				WellHorIntersectFinder(const Well::Track&,
						const Well::D2TModel* =nullptr);
				//! a d2t model is needed if z is time
				~WellHorIntersectFinder();

    void			setHorizon(const EM::ObjectID& emid);

    float			findZIntersection() const;
				//return undef if not found

protected:

    const Well::Track&		track_;
    const Well::D2TModel*	d2t_;
    const EM::Horizon2D*	hor2d_;
    const EM::Horizon3D*	hor3d_;

    float			intersectPosHor(const Coord3&) const;
};
