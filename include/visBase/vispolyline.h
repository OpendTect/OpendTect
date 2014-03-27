#ifndef vispolyline_h
#define vispolyline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visshape.h"
#include "position.h"
#include "draw.h"


class LineStyle;


namespace osgGeo {
    class PolyLineNode;
}

namespace visBase
{

class DrawStyle;

/*!\brief


*/

mExpClass(visBase) PolyLine : public VertexShape
{
public:
    static PolyLine*	create()
    			mCreateDataObj(PolyLine);

    int 		size() const;
    void		addPoint( const Coord3& pos );
    Coord3		getPoint( int ) const;
    void		setPoint( int, const Coord3& );
    void		removePoint( int );
    void		removeAllPoints();
    void		setLineStyle(const LineStyle&);
    const LineStyle&	lineStyle() const;

    void		setDisplayTransformation( const mVisTrans* );
    			/*!<\note The transformation is forwarded to the
			     the coordinates, if you change coordinates,
			     you will have to setTransformation again.  */

protected:
    					~PolyLine();
    DrawStyle*				drawstyle_;
    Geometry::RangePrimitiveSet*	coordrange_;
};



mExpClass(visBase) PolyLine3D : public VertexShape
{
public:
    static PolyLine3D*	create()
			mCreateDataObj(PolyLine3D);

    void		setLineStyle(const LineStyle&);
    const LineStyle&	lineStyle() const;
    void		setResolution(int);
    int			getResolution() const;
    void		addPrimitiveSetToScene(osg::PrimitiveSet*);
    void		removePrimitiveSetFromScene(const osg::PrimitiveSet*);
    void		touchPrimitiveSet(int);
    void		setCoordinates(Coordinates*);
    void		setDisplayTransformation( const mVisTrans* );
    			/*!<\note The transformation is forwarded to the
			     the coordinates, if you change coordinates,
			     you will have to setTransformation again.  */

protected:
    osgGeo::PolyLineNode*	osgpoly_;
    LineStyle			lst_;
};


}; // Namespace


#endif

