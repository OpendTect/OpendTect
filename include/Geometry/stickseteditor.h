#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "geeditor.h"

namespace Geometry
{
class FaultStickSet;

mExpClass(Geometry) StickSetEditor : public ElementEditor
{
public:
		StickSetEditor( Geometry::FaultStickSet& );
		~StickSetEditor();

    bool	mayTranslate2D( GeomPosID ) const override;
    Coord3	translation2DNormal( GeomPosID ) const override;

protected:
    void	addedKnots(CallBacker*);
};

} // namespace Geometry
