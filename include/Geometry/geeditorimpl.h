#ifndef geeditorimpl_h
#define geeditorimpl_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: geeditorimpl.h,v 1.1 2005-01-06 09:44:18 kristofer Exp $
________________________________________________________________________

-*/

#include "geeditor.h"

namespace Geometry
{

class TrackPlane;


class ElementEditorImpl : public ElementEditor
{
public:
    		ElementEditorImpl( Element& elem,
			const Coord3& dir1d=Coord3::udf(),
			const Coord3& norm2d=Coord3::udf(),
			bool allow3d=false )
		    : ElementEditor( elem )
		    , translate1ddir( dir1d )
		    , translation2dnormal( norm2d )
		    , maytranslate3d( allow3d )
		{}

    bool	mayTranslate1D( GeomPosID ) const
    		{ return translate1ddir.isDefined(); }
    Coord3	translation1DDirection( GeomPosID ) const
		{ return translate1ddir; }

    bool	mayTranslate2D( GeomPosID ) const
    		{ return translation2dnormal.isDefined(); }
    Coord3	translation2DNormal( GeomPosID ) const
		{ return translation2dnormal; }

    bool	mayTranslate3D( GeomPosID ) const { return maytranslate3d; }

protected:
    Coord3	translate1ddir;
    Coord3	translation2dnormal;
    bool	maytranslate3d;
};


class BinIDElementEditor : public ElementEditorImpl
{
public:
	    BinIDElementEditor( Geometry::Element& elem)
		: ElementEditorImpl( elem,  Coord3(0,0,1) ) {}
};


class PlaneElementEditor : public ElementEditorImpl
{
public:
	    PlaneElementEditor( Element& elem, const Coord3& normal )
		: ElementEditorImpl( elem,  Coord3::udf(), normal ) {}
};
};

#endif

