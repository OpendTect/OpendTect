#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "draw.h"
#include "selector.h"
#include "thread.h"
#include "viscamera.h"
#include "visdrawstyle.h"
#include "visobject.h"
#include "vistransform.h"


namespace osgGeo{ class PolygonSelection; }

template <class T> class ODPolygon;

namespace visBase
{
class Material;
class SelectionCallBack;
class DrawStyle;
class Camera;

/*!
Paints a polygon or a rectangle just in front of near-clipping plane driven
by mouse- movement. Once drawn, queries can be made whether points are
inside or outside the polygon.
*/

mExpClass(visBase) PolygonSelection : public VisualObjectImpl
{
public:
    static RefMan<PolygonSelection> create();
				mCreateDataObj(PolygonSelection);

    enum			SelectionType { Off, Rectangle, Polygon };
    void			setSelectionType(SelectionType);
    SelectionType		getSelectionType() const;

    void			setLineStyle(const OD::LineStyle&);
    const OD::LineStyle&	getLineStyle() const;

    void			clear();
    bool			hasPolygon() const;
    bool			singleSelection() const;
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


    void			setUTMCoordinateTransform(const mVisTrans*);
				/*!< This is a special function to set transform
				     for computing real-world coordinates from
				     screen coordinates.
				*/

    static Notifier<PolygonSelection>& polygonFinished();

    bool			rayPickThrough(const Coord3& worldpos,
					       TypeSet<int>& pickedobjids,
					       int depthidx=0) const;
    void			setPrimaryCamera(Camera*);
    void			setHUDCamera(Camera*);
    PolygonSelection*		copy() const;

protected:
				PolygonSelection(
				    const osgGeo::PolygonSelection*);
				~PolygonSelection();

    void			polygonChangeCB(CallBacker*);


    ConstRefMan<mVisTrans>		utm2disptransform_;

    RefMan<DrawStyle>			drawstyle_;
    mutable ODPolygon<double>*		polygon_ = nullptr;
    mutable Threads::ReadWriteLock	polygonlock_;


    osgGeo::PolygonSelection*		selector_;
    RefMan<Camera>			primarycamera_;
    SelectionCallBack*			selectorcb_ = nullptr;

public:
    mDeprecated("Use setPrimaryCamera")
    void			setMasterCamera(Camera*);
};


mExpClass(visBase) PolygonCoord3Selector : public Selector<Coord3>
{
public:
				PolygonCoord3Selector(const PolygonSelection&);
				~PolygonCoord3Selector();

    Selector<Coord3>*		clone() const override;
    const char*			selectorType() const override;
    bool			isOK() const override;
    bool			hasPolygon() const;
    bool			includes(const Coord3&) const override;
    bool			canDoRange() const override	{ return true; }
    char			includesRange(const Coord3& start,
					     const Coord3& stop) const override;
    void			copySelection(const PolygonSelection&);

protected:
    bool			isEq(const Selector<Coord3>&) const override;

    ConstRefMan<PolygonSelection> vissel_;
};

} // namespace visBase
