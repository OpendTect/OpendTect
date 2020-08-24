#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2008
________________________________________________________________________

-*/

#include <QGraphicsItem>
#include <QPolygonF>
#include <QString>
#include <QTextOption>
#include <QMutex>
#include <QWaitCondition>
#include <QFont>

#include "draw.h"
#include "notify.h"

class uiPixmap;

static int ODGraphicsType = 100000;

class ODGraphicsHighlightItem
{
public:
    virtual void		highlight()	{}
    virtual void		unHighlight()	{}

    virtual void		setQPen(const QPen&)	{}
};


class ODGraphicsPointItem : public QAbstractGraphicsShapeItem
{
public:
				ODGraphicsPointItem();

    QRectF			boundingRect() const;
    void			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*);

    void			drawPoint(QPainter*);
    void			setHighLight( bool hl )
				{ highlight_ = hl ; }

    virtual int			type() const	{ return ODGraphicsType+1; }

protected:
    virtual void		mouseMoveEvent(QGraphicsSceneMouseEvent*);
    bool			highlight_;
};



class ODGraphicsMarkerItem : public QAbstractGraphicsShapeItem
{
public:
				ODGraphicsMarkerItem();
    virtual			~ODGraphicsMarkerItem();

    QRectF			boundingRect() const;
    void			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*);
    static void			drawMarker(QPainter&,OD::MarkerStyle2D::Type,
					   float,float);

    void			setMarkerStyle(const OD::MarkerStyle2D&);
    void			setFill( bool fill )	  { fill_ = fill; }
    void			setFillColor( const Color& col )
				{ fillcolor_ = col; }
    void			setSideLength( int side ) { side_ = side; }

    virtual int			type() const	{ return ODGraphicsType+2; }
    const OD::MarkerStyle2D*	getMarkerStyle() { return mstyle_; }

protected:
    virtual void		mouseMoveEvent(QGraphicsSceneMouseEvent*);
    QRectF			boundingrect_;
    OD::MarkerStyle2D*		mstyle_;
    Color			fillcolor_;
    bool			fill_;
    int				side_;
};


class ODGraphicsPixmapItem : public QGraphicsPixmapItem
{
public:
				ODGraphicsPixmapItem();
				ODGraphicsPixmapItem(const uiPixmap&);

    void			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*);

    virtual int			type() const	{ return ODGraphicsType+3; }
    void			setPaintInCenter(bool);

protected:
    virtual void		mouseMoveEvent(QGraphicsSceneMouseEvent*);
    virtual void		hoverEnterEvent(QGraphicsSceneHoverEvent*);
    virtual void		hoverLeaveEvent(QGraphicsSceneHoverEvent*);

    bool			paintincenter_;

};


class ODGraphicsArrowItem : public QAbstractGraphicsShapeItem
{
public:
				ODGraphicsArrowItem();

    QRectF			boundingRect() const;
    void			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*);

    void			drawArrow(QPainter&);
    double			getAddedAngle(double,float);
    QPoint			getEndPoint(const QPoint&,double,double);
    void			drawArrowHead(QPainter&,const QPoint&,
					      const QPoint&);
    void			setArrowStyle( const OD::ArrowStyle& arrowstyle)
				{ arrowstyle_ = arrowstyle ; }
    void			setArrowSize( const int arrowsz )
				{ arrowsz_ = arrowsz ; }
    void			setLineStyle(QPainter&,const OD::LineStyle&);

    virtual int			type() const	{ return ODGraphicsType+4; }

protected:
    virtual void		mouseMoveEvent(QGraphicsSceneMouseEvent*);

    OD::ArrowStyle		arrowstyle_;
    int				arrowsz_;
};


class ODGraphicsTextItem : public QAbstractGraphicsShapeItem
{
public:
				ODGraphicsTextItem();

    QRectF			boundingRect() const;
    void			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*);

    void			setText(const QString&);
    void			setFont(const QFont&);
    QFont			getFont() const;

    void			setVAlignment(const Qt::Alignment&);
    void			setHAlignment(const Qt::Alignment&);

    virtual int			type() const	{ return ODGraphicsType+5; }

protected:
    virtual void		mouseMoveEvent(QGraphicsSceneMouseEvent*);
    QPointF			getAlignment() const;

    QFont			font_;
    QString			text_;
    Qt::Alignment		hal_;
    Qt::Alignment		val_;
};


class ODGraphicsAdvancedTextItem : public QGraphicsTextItem
{
public:
				ODGraphicsAdvancedTextItem(bool);

protected:
    virtual void		mouseMoveEvent(QGraphicsSceneMouseEvent*);
};


class ODGraphicsPathItem : public QGraphicsPathItem
			 , public ODGraphicsHighlightItem
{
public:
				ODGraphicsPathItem();
				~ODGraphicsPathItem();

    void			set(const QPainterPath&);
    QPainterPath		shape() const;

    void			setQPen(const QPen& pen);
    void			highlight();
    void			unHighlight();

protected:
    QPainterPath		path_;
    QPen&			mypen_;
};


class ODGraphicsPolyLineItem : public QAbstractGraphicsShapeItem
			     , public ODGraphicsHighlightItem
{
public:
				ODGraphicsPolyLineItem();
				~ODGraphicsPolyLineItem();

    QRectF			boundingRect() const;
    void			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*);
    QPainterPath		shape() const;

    void			setPolyLine(const QPolygonF&,bool closed);
    void			setFillRule(Qt::FillRule);
    bool			isEmpty() const;
    void			setEmpty();

    void			setQPen(const QPen& pen);
    void			highlight();
    void			unHighlight();

    virtual int			type() const	{ return ODGraphicsType+6; }

protected:
    virtual void		mouseMoveEvent(QGraphicsSceneMouseEvent*);

    bool			closed_;
    QPolygonF			qpolygon_;
    Qt::FillRule		fillrule_;
    QPainterPath		path_;
    QPen&			mypen_;
};


class ODGraphicsMultiColorPolyLineItem : public QAbstractGraphicsShapeItem
				       , public ODGraphicsHighlightItem
{
public:
				ODGraphicsMultiColorPolyLineItem();
				~ODGraphicsMultiColorPolyLineItem();

    QRectF			boundingRect() const;
    void			paint(QPainter*,
				      const QStyleOptionGraphicsItem*,
				      QWidget*);
    QPainterPath		shape() const;

    void			setPolyLine(const QPolygonF&);
    void			setQPens(const QVector<QPen>&);
    void			setPenWidth(int);

    void			highlight()	{ highlight_ = true; }
    void			unHighlight()	{ highlight_ = false; }

    virtual int			type() const	{ return ODGraphicsType+7; }

protected:

				class ODLineSegment
				{
				public:
				    QPolygonF		qpolygon_;
				    QVector<QPen>	qpens_;

				    bool		isEmpty() const;
				    void		add(const QPointF&,
							    const QPen&);
				    void		setWidth(int);
				};

    virtual void		mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void			cleanupPolygon();

    bool			highlight_;
    QPolygonF			inputqpolygon_;
    QVector<QPen>		inputqpens_;
    QVector<ODLineSegment>	odlinesegments_;
    QRectF			brect_;
    QPainterPath		path_;
};


class ODGraphicsLineItem : public QGraphicsItem , public ODGraphicsHighlightItem
{
public:
				ODGraphicsLineItem();
				~ODGraphicsLineItem();

    QRectF			boundingRect() const;
    void			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*);

    void			setLine(qreal,qreal,qreal,qreal);
    void			setLine(const QLineF&);

    void			setQPen(const QPen&);
    const QPen&			getQpen() const { return qpen_; }

    void			highlight()	{ highlight_ = true; }
    void			unHighlight()	{ highlight_ = false; }

    virtual int			type() const	{ return ODGraphicsType+8; }

protected:

    virtual void		mouseMoveEvent(QGraphicsSceneMouseEvent*);

    QLineF			qline_;
    bool			highlight_;
    QPen&			qpen_;
};


class ODGraphicsItemGroup : public QGraphicsItemGroup
{
public:
				ODGraphicsItemGroup();

    QRectF			boundingRect() const;
    void			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*);
protected:
    virtual void		mouseMoveEvent(QGraphicsSceneMouseEvent*);

};


class ODGraphicsDynamicImageItem : public QGraphicsItem, public CallBacker
{
public:
				ODGraphicsDynamicImageItem();

    QRectF			boundingRect() const { return bbox_; }
    void			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*);

    void			setImage(bool isdynamic,const QImage&,
					 const QRectF&);
    bool			updateResolution(const QPainter*);
    const QRectF&		wantedWorldRect() const;
    const QSize&		wantedScreenSize() const;

    virtual int			type() const	{ return ODGraphicsType+9; }

    Notifier<ODGraphicsDynamicImageItem>	wantsData;
    bool					isSnapshot() const;
						/*!<If set during a wantsData
						   trigger, the delivered image
						   must be of same size as
						   requested image. */

protected:
    virtual void		mouseMoveEvent(QGraphicsSceneMouseEvent*);

    QRectF			wantedwr_;
    QSize			wantedscreensz_;

    QMutex			imagelock_;
    QWaitCondition		imagecond_;
    bool			updatedynpixmap_;
    QImage			dynamicimage_;
    QRectF			dynamicimagebbox_;
    bool			dynamicrev_[2];
    bool			updatebasepixmap_;
    QImage			baseimage_;
    QRectF			bbox_;
    bool			baserev_[2];
    bool			issnapshot_;

    PtrMan<QPixmap>		basepixmap_;	//Only access in paint
    PtrMan<QPixmap>		dynamicpixmap_; //Only access in paint
    QRectF			dynamicpixmapbbox_; //Only access in paint

};
