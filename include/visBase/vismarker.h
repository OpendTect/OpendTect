#ifndef vismarker_h
#define vismarker_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Nanne Hemstra
 Date:		July 2002
 RCS:		$Id: vismarker.h,v 1.4 2002-10-23 09:41:55 nanne Exp $
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

/*!\brief

Marker is a basic pickmarker with a constant size on screen. 
Size and shape are settable.

*/

class Marker : public VisualObjectImpl
{
public:
    static Marker*	create()
			mCreateDataObj0arg(Marker);

			~Marker();

    enum Type		{ Cube, Cone, Cylinder, Sphere, Cross, Star };

    void		setCenterPos(const Coord3&);
    Coord3		centerPos() const;
   
    void		setType(Type);
    Type		getType() const		{ return markertype; }
 
    void		setSize(const float);
    float		getSize() const;

    void		setScale(const Coord3&);

    int			usePar(const IOPar&);
    void		fillPar(IOPar&,TypeSet<int>&) const;

protected:

    ShapeScale*		shapescale;
    SoTranslation*	position;
    SoScale*		scale;
    SoGroup*		group;

    Type		markertype;

    static const char*  centerposstr;
};

};


#endif
