#ifndef vismarker_h
#define vismarker_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Nanne Hemstra
 Date:		July 2002
 RCS:		$Id: vismarker.h,v 1.1 2002-07-25 15:25:51 nanne Exp $
________________________________________________________________________


-*/

#include "visobject.h"

class SoTranslation;
class SoGroup;
class SoScale;

namespace Geometry { class Pos; };

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

    void		setCenterPos(const Geometry::Pos&);
    Geometry::Pos	centerPos() const;
   
    void		setType(Type);
    Type		getType() const		{ return markertype; }
 
    void		setSize(const float);
    float		getSize() const;

    void		setScale(const Geometry::Pos&);

    int			usePar(const IOPar&);
    void		fillPar(IOPar&,TypeSet<int>&) const;

protected:

    ShapeScale*		shapescale;
    SoTranslation*	position;
    SoScale*		scale;
    SoGroup*		group;

    Type		markertype;
};

};


#endif
