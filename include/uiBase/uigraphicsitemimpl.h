#ifndef uigraphicsitemimpl_h
#define uigraphicsitemimpl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		April 2008
 RCS:		$Id: uigraphicsitemimpl.h,v 1.12 2009-04-01 11:46:22 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigraphicsitem.h"
#include "uigeom.h"

class Alignment;
class ArrowStyle;
class Color;
class LineStyle;
class MarkerStyle2D;
class ioPixmap;
class uiFont;
class uiRect;

class QGraphicsItem;
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QGraphicsPixmapItem;
class QGraphicsPolygonItem;
class QGraphicsRectItem;
class QGraphicsTextItem;
class ODGraphicsPolyLineItem;

class ODGraphicsArrowItem;
class ODGraphicsMarkerItem;
class ODGraphicsPixmapItem;
class ODGraphicsPointItem;
template <class T> class ODPolygon;


mClass uiEllipseItem : public uiGraphicsItem
{
public:
				uiEllipseItem(const uiPoint& center,
					      const uiSize&);
				uiEllipseItem(const uiSize&);
				uiEllipseItem();
				~uiEllipseItem();

    QGraphicsEllipseItem*	qEllipseItem()	{ return qellipseitem_; }
    void			setSize(const uiSize&);

protected:

    QGraphicsItem*		mkQtObj();
    QGraphicsEllipseItem*	qellipseitem_;
};


mClass uiCircleItem : public uiEllipseItem
{
public:
    				uiCircleItem(const uiPoint& center,int r);
				uiCircleItem(int r);

    void			setRadius(int);
};


mClass uiLineItem : public uiGraphicsItem
{
public:
    			uiLineItem();
    			uiLineItem(const uiPoint&,const uiPoint&);
    			uiLineItem(QGraphicsLineItem*);
			~uiLineItem();

    QGraphicsLineItem* qLineItem()	{ return qlineitem_; }
    void 		setPenStyle(const LineStyle&);
    void		setPenColor(const Color&);
    void		setLine(float x1,float y1,float x2,float y2);
    void		setLine(const uiPoint& x,const uiPoint& y);
    uiRect*		lineRect();

protected:

    QGraphicsItem*	mkQtObj();
    QGraphicsLineItem*	qlineitem_;
};


mClass uiPixmapItem : public uiGraphicsItem
{
public:
    				uiPixmapItem();
    				uiPixmapItem(const ioPixmap&);
    				uiPixmapItem(ODGraphicsPixmapItem*);
				~uiPixmapItem();

    ODGraphicsPixmapItem*	qPixmapItem()	{ return qpixmapitem_; }
    void			setOffset(int left,int top);
    void			setPixmap(const ioPixmap&);
    const uiPoint&		transformToScene( float x, float y );

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsPixmapItem*	qpixmapitem_;
};


mClass uiPolygonItem : public uiGraphicsItem
{
public:
    			uiPolygonItem();
    			uiPolygonItem(const TypeSet<uiPoint>&,bool fill);
    			uiPolygonItem(const ODPolygon<int>&,bool fill);
    			uiPolygonItem(QGraphicsPolygonItem*);
			~uiPolygonItem();

    QGraphicsPolygonItem* qPolygonItem()	{ return qpolygonitem_; }
    void		fill();
    void		setPolygon(const TypeSet<uiPoint>&);
    void		setPolygon(const ODPolygon<int>&);

protected:

    QGraphicsItem*	mkQtObj();
    QGraphicsPolygonItem* qpolygonitem_;
};


mClass uiPolyLineItem : public uiGraphicsItem
{
public:
    				uiPolyLineItem();
    				uiPolyLineItem(const TypeSet<uiPoint>&);
				~uiPolyLineItem();

    ODGraphicsPolyLineItem* 	qPolyLineItem()
    				{ return qpolylineitem_; }
    void			setPolyLine(const TypeSet<uiPoint>&);

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsPolyLineItem*	qpolylineitem_;
};



mClass uiRectItem : public uiGraphicsItem
{
public:
    			uiRectItem();
    			uiRectItem(int x, int y, int width, int height);
    			uiRectItem(QGraphicsRectItem*);
			~uiRectItem();

    QGraphicsRectItem*  qRectItem()	{ return qrectitem_; }
    void		setRect(int x, int y, int width, int height); 

protected:

    QGraphicsItem*	mkQtObj();
    QGraphicsRectItem*	qrectitem_;
};


mClass uiTextItem : public uiGraphicsItem
{
public:
    			uiTextItem();
    			uiTextItem(int x,int y,const char*,const Alignment&);
    			uiTextItem(QGraphicsTextItem*);
			~uiTextItem();

    QGraphicsTextItem*  qTextItem()	{ return qtextitem_; }
    void 		setFont(const uiFont&);
    int			getTextWidth();
    const uiRect*	getTextRect();
    void 		setAlignment(const Alignment&);
    void 		setText(const char*); 
    void		setTextColor(const Color&);

protected:

    QGraphicsItem*	mkQtObj();
    QGraphicsTextItem*	qtextitem_;
};


mClass uiMarkerItem : public uiGraphicsItem
{
public:
				uiMarkerItem(const MarkerStyle2D&);
				uiMarkerItem(ODGraphicsMarkerItem*);
				~uiMarkerItem();

    ODGraphicsMarkerItem*  	qMarkerItem()	{ return qmarkeritem_; }
    void			setMarkerStyle(const MarkerStyle2D&);
    void			setFill(bool);

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsMarkerItem*	qmarkeritem_;
};


mClass uiPointItem : public uiGraphicsItem
{
public:
    				uiPointItem();
    				uiPointItem(ODGraphicsPointItem*);
				~uiPointItem();

    ODGraphicsPointItem*	qPointItem()		{ return qpointitem_; }

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsPointItem*	qpointitem_;
};


mClass uiArrowItem : public uiGraphicsItem
{
public:
    				uiArrowItem();
    				uiArrowItem(ODGraphicsArrowItem*);
				~uiArrowItem();

    void                        setArrowStyle(const ArrowStyle&);
    void 			setArrowSize(int);

    ODGraphicsArrowItem*  	qArrowItem()	{ return qarrowitem_; }

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsArrowItem*	qarrowitem_;
};


#endif
