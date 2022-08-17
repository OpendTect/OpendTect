#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		November 2008
 Contents:	Ranges
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
