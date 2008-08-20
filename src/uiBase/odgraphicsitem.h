#ifndef odgraphicsitem_h
#define odgraphicsitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		April 2008
 RCS:		$Id: odgraphicsitem.h,v 1.1 2008-08-20 03:33:48 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "qgraphicsitem.h"
#include "uienums.h"
#include "draw.h"

class QStyleOptionGraphicsItem;
class QWidget;
class QPoint;
class QPainter;
class QString;
class QTextOption;

class Color;
class MarkerStyle2D;
class LineStyle;

class ODGraphicsPointItem : public QAbstractGraphicsShapeItem
{
public:
    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void 			drawPoint(QPainter*);
    void			setHighLight( bool hl )
				{ highlight_ = hl ; }
protected:
    bool			highlight_;
};


/*class uiGraphicsShapeItem : public QAbstractGraphicsShapeItem
{
public:
}*/

class ODGraphicsMarkerItem : public QAbstractGraphicsShapeItem
{
public:
    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void 			drawMarker(QPainter&);
    void			setMarkerStyle( const MarkerStyle2D& mstyle )
    				{ mstyle_ = &mstyle ; }
    void			setAngle( float angle )	  { angle_ = angle; }
    void			setSideLength( int side ) { side_ = side; }

protected:
    QRectF			boundingrect_;
    const MarkerStyle2D*	mstyle_;	
    float			angle_;	
    int 			side_;	
    const QPoint* 		qpoint_;	
};


class ODGraphicsArrowItem : public QAbstractGraphicsShapeItem
{
public:
    				ODGraphicsArrowItem();
    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void 			drawArrow(QPainter&);
    double 			getAddedAngle(double,float);
    QPoint 			getEndPoint(const QPoint&,double,double);
    void 			drawArrowHead(QPainter&,const QPoint&,
	    				      const QPoint&);
    void			setArrowStyle( const ArrowStyle& arrowstyle )
    				{ arrowstyle_ = arrowstyle ; }
    void			setArrowSize( const int arrowsz )
    				{ arrowsz_ = arrowsz ; }
    void			setLineStyle(QPainter&,const LineStyle&);

protected:
    QRectF*			boundingrect_;
    ArrowStyle			arrowstyle_;
    QPoint*			qpointhead_;
    QPoint*			qpointtail_;
    int				arrowsz_;
};


class ODGraphicsTextItem : public QGraphicsTextItem
{
public:
    				ODGraphicsTextItem();
    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void 			setTextAlignment(OD::Alignment);
    void			setText(const char*);
protected:
    QRectF			boundingrect_;
    QString*			text_;
    QTextOption*		alignoption_;
};

#endif

