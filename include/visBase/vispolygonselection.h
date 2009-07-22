#ifndef vispolygonselection_h
#define vispolygonselection_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		June 2008
 RCS:		$Id: vispolygonselection.h,v 1.6 2009-07-22 16:01:25 cvsbert Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "selector.h"
#include "draw.h"
#include "thread.h"

class SoPolygonSelect;
class SoSeparator;
template <class T> class ODPolygon;

namespace visBase
{
class Material;
class DrawStyle;

/*!
Paints a polygon or a rectangle just in front of near-clipping plane driven
by mouse- movement. Once drawn, queries can be made whether points are
inside or outside the polygon.
*/

mClass PolygonSelection : public VisualObjectImpl
{
public:
    static PolygonSelection*	create()
				mCreateDataObj(PolygonSelection);

    enum			SelectionType { Off, Rectangle, Polygon };
    void			setSelectionType(SelectionType);
    SelectionType		getSelectionType() const;

    void			setLineStyle(const LineStyle&);
    const LineStyle&		getLineStyle() const;

    bool			hasPolygon() const;
    bool			isSelfIntersecting() const;
    bool			isInside(const Coord3&,
	    				 bool displayspace=false) const;
    char			includesRange(const Coord3& start,
	    				      const Coord3& stop,
					      bool displayspace ) const;

    void			setDisplayTransformation( Transformation* );
    Transformation*		getDisplayTransformation();


protected:

    static void				polygonChangeCB(void*, SoPolygonSelect*);
					~PolygonSelection();
    Transformation*			transformation_;

    DrawStyle*				drawstyle_;
    mutable ODPolygon<double>*		polygon_;
    mutable Threads::ReadWriteLock	polygonlock_;

    SoPolygonSelect*			selector_;
};


mClass PolygonCoord3Selector : public Selector<Coord3>
{
public:
				PolygonCoord3Selector(const PolygonSelection&);
				~PolygonCoord3Selector();

    Selector<Coord3>*		clone() const;
    const char*			selectorType() const;
    bool			isOK() const;
    bool			hasPolygon() const;
    bool			includes(const Coord3&) const;
    bool			canDoRange() const	{ return true; }
    char			includesRange(const Coord3& start,
	    				      const Coord3& stop) const;

protected:
    bool			isEq(const Selector<Coord3>&) const;

    const PolygonSelection&	vissel_;
};

};


#endif
