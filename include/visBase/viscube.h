#ifndef viscube_h
#define viscube_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscube.h,v 1.7 2002-04-30 14:13:28 kristofer Exp $
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

    int			usePar( const IOPar& );
    void		fillPar( IOPar&, TypeSet<int>& ) const;

protected:
    SoCube*		cube;
    SoTranslation*	position;
    static const char*	centerposstr;
    static const char*	widthstr;
};

};


#endif
