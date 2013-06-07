#ifndef wellhorpos_h
#define wellhorpos_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jul 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "emposid.h"
#include "namedobj.h"
#include "position.h"

/*!brief used to give well / horizon intersection. In theory more than one intersection is possible (in case of faults or deviated tracks along the horizon ) but only one pos will be returned!*/

namespace Well { class Track; class D2TModel; }
namespace EM { class Horizon2D; class Horizon3D; }

mClass WellHorIntersectFinder
{
public:
    				WellHorIntersectFinder(const Well::Track&,
						const Well::D2TModel* d2t=0);
				//! a d2t model is needed if z is time 

    void			setHorizon(const EM::ObjectID& emid);

    float			findZIntersection() const;
   				//return undef if not found else z pos (in s)
    				//of the intersect.

protected:

    const Well::Track&		track_;
    const Well::D2TModel*	d2t_;
    const EM::Horizon2D*	hor2d_;
    const EM::Horizon3D*	hor3d_;

    float			intersectPosHor(const Coord3&) const;
};

#endif
