#ifndef viscube_h
#define viscube_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscube.h,v 1.4 2002-03-11 10:46:12 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"

class SoCube;
class SoTranslation;

namespace Geometry { class Pos; };

namespace visBase
{
class Scene;

/*!\brief

Cube is a basic cube that is settable in size.

*/

class Cube : public VisualObjectImpl
{
public:
    static Cube*	create()
			mCreateDataObj0arg(Cube);

    void		setCenterPos( const Geometry::Pos& );
    Geometry::Pos	centerPos() const;
    
    void		setWidth( const Geometry::Pos& );
    Geometry::Pos	width() const;

protected:
    			Cube();

    SoCube*		cube;
    SoTranslation*	position;
};

};


#endif
