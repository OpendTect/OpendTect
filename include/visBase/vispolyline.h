#ifndef vispolyline_h
#define vispolyline_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispolyline.h,v 1.9 2004-07-23 12:58:17 kristofer Exp $
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
};


class IndexedPolyLine3D	: public IndexedShape
{
public:
    static IndexedPolyLine3D*	create()
				mCreateDataObj(IndexedPolyLine3D);

    float			getRadius() const;
    void			setRadius(float);
};


}; // Namespace


#endif
