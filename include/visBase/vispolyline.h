#ifndef vispolyline_h
#define vispolyline_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispolyline.h,v 1.7 2003-09-22 08:29:52 kristofer Exp $
________________________________________________________________________


-*/

#include "visshape.h"
#include "position.h"

class SoLineSet;
class SoIndexedLineSet;

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
    void		setPoint( int, const Coord3& );
    void		removePoint( int );

protected:
    SoLineSet*		lineset;
};


class IndexedPolyLine	: public IndexedShape
{
public:
    static IndexedPolyLine*	create()
				mCreateDataObj(IndexedPolyLine);

protected:
    SoIndexedLineSet*	lineset;
};

}; // Namespace


#endif
