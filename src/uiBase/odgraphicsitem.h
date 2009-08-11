#ifndef odgraphicsitem_h
#define odgraphicsitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2008
 RCS:		$Id: odgraphicsitem.h,v 1.10 2009-08-11 07:07:14 cvssatyaki Exp $
________________________________________________________________________

-*/

#include <QGraphicsItem>
#include <QPolygon>

#include "draw.h"

class QTextOption;
class ioPixmap;

class ODGraphicsPointItem : public QAbstractGraphicsShapeItem
{
public:
    				ODGraphicsPointItem()
				    : highlight_(false)
				    , penwidth_(2)
				    , pencolor_(Color::Black())	{}

    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void 			drawPoint(QPainter*);
    void			setHighLight( bool hl )
				{ highlight_ = hl ; }
    void			setColor( const Color& col )
				{ pencolor_ = col ; }

protected:
    bool			highlight_;
    int				penwidth_;
    Color			pencolor_;
};



class ODGraphicsMarkerItem : public QAbstractGraphicsShapeItem
{
public:
    				ODGraphicsMarkerItem();

    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void 			drawMarker(QPainter&);
    void			setMarkerStyle(const MarkerStyle2D&);
    void			setFill( bool fill )	  { fill_ = fill; }
    void			setSideLength( int side ) { side_ = side; }

protected:
    QRectF			boundingrect_;
    MarkerStyle2D*		mstyle_;	
    bool			fill_;	
    int 			side_;	

public:
    				~ODGraphicsMarkerItem();
};


class ODGraphicsPixmapItem : public QGraphicsPixmapItem
{
public:
    				ODGraphicsPixmapItem();
    				ODGraphicsPixmapItem(const ioPixmap&);
    void                        paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*);
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
    int				arrowsz_;
};


class ODGraphicsTextItem : public QGraphicsTextItem
{
public:
    				ODGraphicsTextItem();
    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void 			setTextAlignment(Alignment);
    void			setText(const char*);
protected:
    QRectF			boundingrect_;
    QString*			text_;
    QTextOption*		alignoption_;

public:
    				~ODGraphicsTextItem();
};


class ODGraphicsPolyLineItem : public QAbstractGraphicsShapeItem
{
public:
    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void			setPolyLine( const QPolygon& polygon )
    				{ qpolygon_ = polygon; }
protected:
    QPolygon			qpolygon_;
};


#endif
