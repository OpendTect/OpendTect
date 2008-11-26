#ifndef uigraphicsitemimpl_h
#define uigraphicsitemimpl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		April 2008
 RCS:		$Id: uigraphicsitemimpl.h,v 1.5 2008-11-26 06:13:35 cvssatyaki Exp $
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


class uiEllipseItem : public uiGraphicsItem
{
public:
				uiEllipseItem();
				uiEllipseItem(QGraphicsEllipseItem*);
				~uiEllipseItem();

    QGraphicsEllipseItem*	qEllipseItem()	{ return qellipseitem_; }
    void			setRect(const uiRect&);

protected:

    QGraphicsItem*		mkQtObj();
    QGraphicsEllipseItem*	qellipseitem_;
};


class uiLineItem : public uiGraphicsItem
{
public:
    			uiLineItem();
			uiLineItem(float x1,float y1,float x2,float y2);
    			uiLineItem(QGraphicsLineItem*);
			~uiLineItem();

    QGraphicsLineItem* qLineItem()	{ return qlineitem_; }
    void 		setPenStyle(const LineStyle&);
    void		setPenColor(const Color&);
    void		setLine(float x1,float y1,float x2,float y2);
    uiRect*		lineRect();

protected:

    QGraphicsItem*	mkQtObj();
    QGraphicsLineItem*	qlineitem_;
};


class uiPixmapItem : public uiGraphicsItem
{
public:
    				uiPixmapItem();
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


class uiPolygonItem : public uiGraphicsItem
{
public:
    			uiPolygonItem();
    			uiPolygonItem(QGraphicsPolygonItem*);
			~uiPolygonItem();

    QGraphicsPolygonItem* qPolygonItem()	{ return qpolygonitem_; }
    void		fill();
    void		setPolygon(const TypeSet<uiPoint>&,bool polyline=false);

protected:

    QGraphicsItem*	mkQtObj();
    QGraphicsPolygonItem* qpolygonitem_;
};


class uiPolyLineItem : public uiGraphicsItem
{
public:
    				uiPolyLineItem();
				~uiPolyLineItem();

    ODGraphicsPolyLineItem* 	qPolyLineItem()
    				{ return qpolylineitem_; }
    void			setPolyLine(const TypeSet<uiPoint>&);

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsPolyLineItem*	qpolylineitem_;
};



class uiRectItem : public uiGraphicsItem
{
public:
    			uiRectItem();
    			uiRectItem(QGraphicsRectItem*);
			~uiRectItem();

    QGraphicsRectItem*  qRectItem()	{ return qrectitem_; }
    void		setRect(int x, int y, int width, int height); 

protected:

    QGraphicsItem*	mkQtObj();
    QGraphicsRectItem*	qrectitem_;
};


class uiTextItem : public uiGraphicsItem
{
public:
    			uiTextItem();
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


class uiMarkerItem : public uiGraphicsItem
{
public:
				uiMarkerItem(const MarkerStyle2D&);
				uiMarkerItem(ODGraphicsMarkerItem*);
				~uiMarkerItem();

    ODGraphicsMarkerItem*  	qMarkerItem()	{ return qmarkeritem_; }
    void			setMarkerStyle(const MarkerStyle2D&);

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsMarkerItem*	qmarkeritem_;
};


class uiPointItem : public uiGraphicsItem
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


class uiArrowItem : public uiGraphicsItem
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
