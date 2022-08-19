#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "faultstickset.h"

namespace Geometry
{

mExpClass(Geometry) FaultStickSurface : public FaultStickSet
{
public:
			FaultStickSurface();

    bool		insertStick(const Coord3& firstpos,
				    const Coord3& editnormal,int stick=0,
				    int firstcol=0) override;

    bool		areSticksVertical() const;
    void		setSticksVertical(bool yn)	{ sticksvertical_=yn; }

protected:

    bool		sticksvertical_;
};

} // namespace Geometry
