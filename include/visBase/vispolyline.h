#ifndef vispolyline_h
#define vispolyline_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispolyline.h,v 1.5 2003-01-21 16:10:00 kristofer Exp $
________________________________________________________________________


-*/

#include "visshape.h"
#include "position.h"

class SoLineSet;

namespace visBase
{

/*!\brief


*/

class PolyLine	: public VertexShape
{
public:
    static PolyLine*	create()
			mCreateDataObj(PolyLine);

    int 		size() const;
    void		addPoint( const Coord3& pos );
    Coord3		getPoint( int ) const;
    void		removePoint( int );

protected:
    SoLineSet*		lineset;
};

}; // Namespace


#endif
