#ifndef vismarker_h
#define vismarker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2002
 RCS:		$Id: vismarker.h,v 1.14 2004-05-11 12:20:24 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "trigonometry.h"
#include "draw.h"

class SoGroup;
class SoShapeScale;
class SoRotation;
class SoShape;
class SoTranslation;

namespace visBase
{
class Transformation;

/*!\brief

Marker is a basic pickmarker with a constant size on screen. 
Size and shape are settable.

*/

class Marker : public VisualObjectImpl
{
public:
    static Marker*	create()
			mCreateDataObj(Marker);

    void		setMarkerStyle(const MarkerStyle3D&);
    const MarkerStyle3D& getMarkerStyle() const	{ return markerstyle; }

    void		setType(MarkerStyle3D::Type);
    MarkerStyle3D::Type	getType() const;
 
    void		setCenterPos(const Coord3&);
    Coord3		centerPos(bool displayspace=false) const;
   
    void		setSize(const float);
    float		getSize() const;

    void		setRotation(const Coord3&,float);
    void		setDirection(const ::Sphere& d)	{ direction = d; }
    const ::Sphere&	getDirection() const		{ return direction; }

    void		setTransformation( Transformation* );
    Transformation*	getTransformation();
    
    int			usePar(const IOPar&);
    void		fillPar(IOPar&,TypeSet<int>&) const;

protected:
			~Marker();
    Transformation*	transformation;

    SoShapeScale*	markerscale;
    SoTranslation*	translation;
    SoShape*		shape;
    SoRotation*		rotation;

    MarkerStyle3D	markerstyle;

    ::Sphere		direction;
    void		setArrowDir(const ::Sphere&);

    static const char*  centerposstr;
};

};


#endif
