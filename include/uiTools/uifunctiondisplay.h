#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2008
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uiaxishandler.h"
#include "uifuncdispbase.h"
#include "uigraphicsview.h"
#include "draw.h"

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
				~uiFunctionDisplay();

    void			setTitle(const uiString&) override;

    Geom::Point2D<float>	getFuncXY(int xpix,bool y2) const;
    Geom::Point2D<float>	getXYFromPix(const Geom::Point2D<int>& pix,
					     bool y2) const;

    Geom::PointF		mapToPosition(const Geom::PointF&,
					      bool y2=false) override;
    Geom::PointF		mapToValue(const Geom::PointF&,
					   bool y2=false) override;

    void			gatherInfo(bool y2=false) override;
    void			draw() override;

    uiObject*			uiobj()		{ return this; }
    const NotifierAccess&	mouseMoveNotifier() override
				{ return mouseMove; }

    Notifier<uiFunctionDisplay>	pointSelected;
    Notifier<uiFunctionDisplay>	pointChanged;
    CNotifier<uiFunctionDisplay,const Geom::PointF&> mouseMove;

    uiAxisHandler*		xAxis() const;
    uiAxisHandler*		yAxis(bool y2) const;

protected:

    uiGraphicsItem*		ypolyitem_;
    uiGraphicsItem*		y2polyitem_;
    uiPolygonItem*		ypolygonitem_;
    uiPolygonItem*		y2polygonitem_;
    uiPolyLineItem*		ypolylineitem_;
    uiPolyLineItem*		y2polylineitem_;
    uiRectItem*			borderrectitem_;
    uiGraphicsItemGroup*	ymarkeritems_;
    uiGraphicsItemGroup*	y2markeritems_;
    uiLineItem*			xmarklineitem_;
    uiLineItem*			ymarklineitem_;
    uiLineItem*			xmarkline2item_;
    uiLineItem*			ymarkline2item_;
    uiTextItem*			titleitem_;

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

    virtual void		drawData();

};


