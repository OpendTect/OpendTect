#ifndef vispolyline_h
#define vispolyline_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispolyline.h,v 1.3 2002-10-14 14:25:26 niclas Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "position.h"

class SoLineSet;
class SoCoordinate3;

namespace visBase
{

/*!\brief


*/

class PolyLine	: public VisualObjectImpl
{
public:
    static PolyLine*	create()
			mCreateDataObj0arg(PolyLine);

    int 		size() const;
    void		addPoint( const Coord3& pos );
    void		insertPoint( int idx, const Coord3& );
    Coord3		getPoint( int ) const;
    void		removePoint( int );

protected:
    SoCoordinate3*	coords;
    SoLineSet*		lineset;
};

}; // Namespace


#endif
