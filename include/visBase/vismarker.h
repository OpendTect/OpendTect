#ifndef vismarker_h
#define vismarker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2002
 RCS:		$Id: vismarker.h,v 1.11 2003-12-11 16:28:16 nanne Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "position.h"
#include "enums.h"

class SoGroup;
class SoMarkerScale;
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

    enum Type		{ Cube, Cone, Cylinder, Sphere, Arrow };
    			DeclareEnumUtils(Type);

    void		setType(Type);
    Type		getType() const		{ return markertype; }
 
    void		setCenterPos(const Coord3&);
    Coord3		centerPos(bool displayspace=false) const;
   
    void		setSize(const float);
    float		getSize() const;

    void		setScale(const Coord3&);

    void		setRotation(const Coord3&,float);
    void		setDirection(const Coord3&);
    const Coord3&	getDirection() const		{ return direction; }
    void		setLength(float);

    void		setTransformation( Transformation* );
    Transformation*	getTransformation();
    
    int			usePar(const IOPar&);
    void		fillPar(IOPar&,TypeSet<int>&) const;

protected:
			~Marker();
    Transformation*	transformation;

    SoMarkerScale*	markerscale;
    SoShape*		shape;
    SoRotation*		rotation;
    Coord3		direction;

    Type		markertype;

    static const char*  centerposstr;
};

};


#endif
