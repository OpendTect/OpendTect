#ifndef vismarker_h
#define vismarker_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Nanne Hemstra
 Date:		July 2002
 RCS:		$Id: vismarker.h,v 1.6 2003-01-20 08:34:08 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "position.h"

class SoTranslation;
class SoGroup;
class SoScale;

namespace visBase
{
class ShapeScale;
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

    enum Type		{ Cube, Cone, Cylinder, Sphere, Cross, Star };
    void		setType(Type);
    Type		getType() const		{ return markertype; }
 
    void		setCenterPos(const Coord3&);
    Coord3		centerPos() const;
   
    void		setSize(const float);
    float		getSize() const;

    void		setScale(const Coord3&);

    void		setTransformation( Transformation* );
    Transformation*	getTransformation();
    
    int			usePar(const IOPar&);
    void		fillPar(IOPar&,TypeSet<int>&) const;

protected:
			~Marker();
    Transformation*	transformation;

    ShapeScale*		shapescale;
    SoTranslation*	position;
    SoScale*		scale;
    SoGroup*		group;

    Type		markertype;

    static const char*  centerposstr;
};

};


#endif
