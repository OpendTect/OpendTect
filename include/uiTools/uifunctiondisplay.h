#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uiaxishandler.h"
#include "uifuncdispbase.h"
#include "uigraphicsview.h"

class uiAxisHandler;
class uiGraphicsItem;
class uiGraphicsItemGroup;
class uiGraphicsScene;
class uiLineItem;
class uiPolygonItem;
class uiPolyLineItem;
class uiRectItem;
class uiTextItem;

/*!\brief displays a function of (X,Y) pairs on a canvas - optionally a Y2.

  * No undefined values supportd (yet). X values can't be undef anyway.
  * You cannot change Setup::editable_ after construction.
  * Y2 is just an optional annotation. It can not be edited, it does not
    determine the X axis scale. Also, no markers are drawn.

 */

mExpClass(uiTools) uiFunctionDisplay :	public uiFuncDispBase,
					public uiGraphicsView
{ mODTextTranslationClass(uiFunctionDisplay)
public:
				uiFunctionDisplay(uiParent*,const Setup&);
				uiFunctionDisplay(uiParent*,const Setup&,
						  OD::Orientation);
				~uiFunctionDisplay();

    void			setTitle(const uiString&) override;
    void			setTitleColor(const OD::Color&);
    void			setTitleAlignment(const Alignment& al);

    Geom::Point2D<float>	getFuncXY(int xpix,bool y2) const;
    Geom::Point2D<float>	getXYFromPix(const Geom::Point2D<int>& pix,
					     bool y2) const;

    Geom::PointF		mapToPosition(const Geom::PointF&,
					      bool y2=false) override;
    Geom::PointF		mapToValue(const Geom::PointF&,
					   bool y2=false) override;

    void			gatherInfo(bool y2=false) override;
    void			draw() override;

    uiObject*			uiobj() override	{ return this; }
    const NotifierAccess&	mouseMoveNotifier() override
				{ return mouseMove; }

    void			allowAddingPoints(bool);

    Notifier<uiFunctionDisplay>	pointSelected;
    Notifier<uiFunctionDisplay>	pointChanged;
    CNotifier<uiFunctionDisplay,const Geom::PointF&> mouseMove;

    uiAxisHandler*		xAxis() const;
    uiAxisHandler*		yAxis(bool y2) const;
    bool			allowaddpts_;
    OD::Orientation		orientation_;

protected:

    uiGraphicsItem*		ypolyitem_		= nullptr;
    uiGraphicsItem*		y2polyitem_		= nullptr;
    uiPolygonItem*		ypolygonitem_		= nullptr;
    uiPolygonItem*		y2polygonitem_		= nullptr;
    uiPolyLineItem*		ypolylineitem_		= nullptr;
    uiPolyLineItem*		y2polylineitem_		= nullptr;
    uiRectItem*			borderrectitem_		= nullptr;
    uiGraphicsItemGroup*	ymarkeritems_		= nullptr;
    uiGraphicsItemGroup*	y2markeritems_		= nullptr;
    uiLineItem*			xmarklineitem_		= nullptr;
    uiLineItem*			ymarklineitem_		= nullptr;
    uiLineItem*			xmarkline2item_		= nullptr;
    uiLineItem*			ymarkline2item_		= nullptr;
    uiTextItem*			titleitem_		= nullptr;

    Geom::Point2D<int>		orientedPix(const MouseEvent& ev) const;

    void			mousePressCB(CallBacker*) override;
    void			mouseReleaseCB(CallBacker*) override;
    void			mouseMoveCB(CallBacker*) override;
    void			mouseDClick(CallBacker*);

    void			cleanUp() override;
    void			setUpAxis(bool y2);
    void			getPointSet(TypeSet<uiPoint>&,bool y2);
    void			drawYCurve(const TypeSet<uiPoint>&);
    void			drawY2Curve(const TypeSet<uiPoint>&,bool havy2);
    void			drawMarker(const TypeSet<uiPoint>&,
					   bool y2=false);
    void			drawMarkLine(uiAxisHandler*,float,OD::Color,
					     uiLineItem*&);
    void			drawBorder();
    void			drawMarkLines() override;
    bool			setSelPt();
    void			reSized( CallBacker* );
    void			saveImageAs( CallBacker* );
    void			init(OD::Orientation);
    bool			isVertical() const;

    virtual void		drawData();
};
