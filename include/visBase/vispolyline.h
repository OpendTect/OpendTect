#ifndef vispolyline_h
#define vispolyline_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispolyline.h,v 1.1 2002-04-25 13:42:23 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "geompos.h"

class SoLineSet;
class SoCoordinate3;

namespace visBase
{

/*!\brief


*/

class PolyLine	: public VisualObjectImpl
{
public:
    static PolyLine*	create(bool selectable)
			mCreateDataObj1arg(PolyLine,bool,selectable);

    int 		size() const;
    void		addPoint( const Geometry::Pos& pos );
    void		insertPoint( int idx, const Geometry::Pos& );
    Geometry::Pos	getPoint( int ) const;
    void		removePoint( int );

protected:
    SoCoordinate3*	coords;
    SoLineSet*		lineset;
};

}; // Namespace


#endif
