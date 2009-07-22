#ifndef vispolyline_h
#define vispolyline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispolyline.h,v 1.15 2009-07-22 16:01:25 cvsbert Exp $
________________________________________________________________________


-*/

#include "visshape.h"
#include "position.h"

class SoLineSet;
class SoIndexedLineSet;
class LineStyle;

namespace visBase
{

class DrawStyle;

/*!\brief


*/

mClass PolyLine	: public VertexShape
{
public:
    static PolyLine*	create()
			mCreateDataObj(PolyLine);

    int 		size() const;
    void		setLineStyle(const LineStyle&);
    void		addPoint( const Coord3& pos );
    Coord3		getPoint( int ) const;
    void		setPoint( int, const Coord3& );
    void		removePoint( int );

protected:
    SoLineSet*		lineset;

    DrawStyle*		drawstyle_;
};


mClass IndexedPolyLine	: public IndexedShape
{
public:
    static IndexedPolyLine*	create()
				mCreateDataObj(IndexedPolyLine);
};


mClass IndexedPolyLine3D	: public IndexedShape
{
public:
    static IndexedPolyLine3D*	create()
				mCreateDataObj(IndexedPolyLine3D);

    float			getRadius() const;
    void			setRadius(float,bool constantonscreen=true,
	    				  float maxworldsize=-1);
    void			setRightHandSystem(bool yn);
    bool			isRightHandSystem() const;
};


}; // Namespace


#endif
