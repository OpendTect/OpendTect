#ifndef viscube_h
#define viscube_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscube.h,v 1.1 2002-02-27 12:40:18 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"

class SoCube;
class SoTranslation;

namespace visBase
{
class Scene;

/*!\brief

Cube is a basic cube that is settable in size.

*/

class Cube : public VisualObject
{
public:

    			Cube(Scene&);

    void		setCenterPos( float, float, float );
    float		centerPos( int dim ) const;
    
    void		setWidth( float, float, float );
    float		width( int dim ) const;

protected:

    SoCube*		cube;
    SoTranslation*	position;
};

};


#endif
