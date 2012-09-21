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

class SoLineSet;
class SoLineSet3D;
class SoIndexedLineSet;
class LineStyle;
class SoMFInt32;

namespace visBase
{

class DrawStyle;

/*!\brief


*/

mClass(visBase) PolyLineBase : public VertexShape
{
public:
    int 		size() const;
    void		addPoint( const Coord3& pos );
    Coord3		getPoint( int ) const;
    void		setPoint( int, const Coord3& );
    void		removePoint( int );
    virtual void	setLineStyle(const LineStyle&) = 0;
    virtual const LineStyle& lineStyle() const = 0;
protected:
    			PolyLineBase(SoVertexShape*);
    SoMFInt32*		numvertices_;
};



mClass(visBase) PolyLine	: public PolyLineBase
{
public:
    static PolyLine*	create()
			mCreateDataObj(PolyLine);

    void		setLineStyle(const LineStyle&);
    const LineStyle&	lineStyle() const;

protected:
    SoLineSet*		lineset_;
    DrawStyle*		drawstyle_;
};


mClass(visBase) PolyLine3D : public PolyLineBase
{
public:
    static PolyLine3D*	create()
			mCreateDataObj(PolyLine3D);

    void		setLineStyle(const LineStyle&);
    const LineStyle&	lineStyle() const;

protected:
    SoLineSet3D*	lineset_;
};


mClass(visBase) IndexedPolyLine	: public IndexedShape
{
public:
    static IndexedPolyLine*	create()
				mCreateDataObj(IndexedPolyLine);
};


mClass(visBase) IndexedPolyLine3D	: public IndexedShape
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

