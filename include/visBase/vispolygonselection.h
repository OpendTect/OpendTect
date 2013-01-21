#ifndef vispolygonselection_h
#define vispolygonselection_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		June 2008
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
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

mExpClass(visBase) PolygonSelection : public VisualObjectImpl
{
public:
    static PolygonSelection*	create()
				mCreateDataObj(PolygonSelection);

    enum			SelectionType { Off, Rectangle, Polygon };
    void			setSelectionType(SelectionType);
    SelectionType		getSelectionType() const;

    void			setLineStyle(const LineStyle&);
    const LineStyle&		getLineStyle() const;

    void			clear();
    bool			hasPolygon() const;
    bool			isSelfIntersecting() const;
    bool			isInside(const Coord3&,
	    				 bool displayspace=false) const;

    char			includesRange(const Coord3& start,
	    				      const Coord3& stop,
					      bool displayspace ) const;
    				/*!< 0: projected box fully outside polygon
				     1: projected box partially outside polygon
				     2: projected box fully inside polygon
				     3: all box points behind projection plane
				     4: some box points behind projection plane
				*/

    void			setDisplayTransformation( const mVisTrans* );
    const mVisTrans*		getDisplayTransformation() const;

    static Notifier<PolygonSelection>* polygonFinished();

    bool			rayPickThrough(const Coord3& worldpos,
					       TypeSet<int>& pickedobjids,
					       int depthidx=0) const;

/*    void			getSelectionRays(TypeSet<Line3D>&) const;*/

protected:

    static void				polygonChangeCB(void*,SoPolygonSelect*);
    static void				paintStopCB(void*,SoPolygonSelect*);

					~PolygonSelection();

    const mVisTrans*			transformation_;

    DrawStyle*				drawstyle_;
    mutable ODPolygon<double>*		polygon_;
    mutable Threads::ReadWriteLock	polygonlock_;

    SoPolygonSelect*			selector_;
};


mExpClass(visBase) PolygonCoord3Selector : public Selector<Coord3>
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

