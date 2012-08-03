#ifndef geeditorimpl_h
#define geeditorimpl_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: geeditorimpl.h,v 1.5 2012-08-03 13:00:27 cvskris Exp $
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "geeditor.h"

namespace Geometry
{

class TrackPlane;


mClass(Geometry) ElementEditorImpl : public ElementEditor
{
public:
    		ElementEditorImpl( Element& elem,
			const Coord3& dir1d=Coord3::udf(),
			const Coord3& norm2d=Coord3::udf(),
			bool allow3d=false );
		~ElementEditorImpl();

    bool	mayTranslate1D( GeomPosID ) const;
    Coord3	translation1DDirection( GeomPosID ) const;

    bool	mayTranslate2D( GeomPosID ) const;
    Coord3	translation2DNormal( GeomPosID ) const;

    bool	mayTranslate3D( GeomPosID ) const;

protected:
    void	addedKnots(CallBacker*);

    Coord3	translate1ddir;
    Coord3	translation2dnormal;
    bool	maytranslate3d;
};


mClass(Geometry) BinIDElementEditor : public ElementEditorImpl
{
public:
	    BinIDElementEditor( Geometry::Element& elem)
		: ElementEditorImpl( elem,  Coord3(0,0,1) ) {}
};


mClass(Geometry) PlaneElementEditor : public ElementEditorImpl
{
public:
	    PlaneElementEditor( Element& elem, const Coord3& normal )
		: ElementEditorImpl( elem,  Coord3::udf(), normal ) {}
};
};

#endif


