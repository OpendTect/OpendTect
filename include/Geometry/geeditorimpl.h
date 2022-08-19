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

class TrackPlane;


mExpClass(Geometry) ElementEditorImpl : public ElementEditor
{
public:
		ElementEditorImpl( Element& elem,
			const Coord3& dir1d=Coord3::udf(),
			const Coord3& norm2d=Coord3::udf(),
			bool allow3d=false );
		~ElementEditorImpl();

    bool	mayTranslate1D( GeomPosID ) const override;
    Coord3	translation1DDirection( GeomPosID ) const override;

    bool	mayTranslate2D( GeomPosID ) const override;
    Coord3	translation2DNormal( GeomPosID ) const override;

    bool	mayTranslate3D( GeomPosID ) const override;

protected:
    void	addedKnots(CallBacker*);

    Coord3	translate1ddir;
    Coord3	translation2dnormal;
    bool	maytranslate3d;
};


mExpClass(Geometry) BinIDElementEditor : public ElementEditorImpl
{
public:
	    BinIDElementEditor( Geometry::Element& elem)
		: ElementEditorImpl( elem,  Coord3(0,0,1) ) {}
};


mExpClass(Geometry) PlaneElementEditor : public ElementEditorImpl
{
public:
	    PlaneElementEditor( Element& elem, const Coord3& normal )
		: ElementEditorImpl( elem,  Coord3::udf(), normal ) {}
};

} // namespace Geometry
