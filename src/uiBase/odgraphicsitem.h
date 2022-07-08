#ifndef odgraphicsitem_h
#define odgraphicsitem_h

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

class uiPixmap;

static int ODGraphicsType = 100000;

class ODGraphicsHighlightItem
{
public:
    virtual void		highlight()		= 0;
    virtual void		unHighlight()		= 0;

    virtual void		setQPen(const QPen&)	{}
};


class ODGraphicsPointItem : public QAbstractGraphicsShapeItem
{
public:
				ODGraphicsPointItem();
				~ODGraphicsPointItem();

    QRectF			boundingRect() const override;
    void			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*) override;

    void			drawPoint(QPainter*);
    void			setHighLight( bool hl )
				{ highlight_ = hl ; }
    void			setColor( const Color& col )
				{ pencolor_ = col ; }

    int				type() const override
				{ return ODGraphicsType+1; }

protected:
    virtual void		mouseMoveEvent(QGraphicsSceneMouseEvent*);
    bool			highlight_;
    int				penwidth_;
    Color			pencolor_;
};



class ODGraphicsMarkerItem : public QAbstractGraphicsShapeItem
{
public:
				ODGraphicsMarkerItem();
    virtual			~ODGraphicsMarkerItem();

    QRectF			boundingRect() const override;
    void			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*) override;
    static void			drawMarker(QPainter&,MarkerStyle2D::Type,
					   float,float);

    const MarkerStyle2D*	getMarkerStyle() { return mstyle_; }
    void			setMarkerStyle(const MarkerStyle2D&);

    void			setFill( bool fill )	  { fill_ = fill; }
    void			setFillColor( const Color& col )
				{ fillcolor_ = col; }
    void			setSideLength( int side ) { side_ = side; }

    int				type() const override
				{ return ODGraphicsType+2; }

protected:
    void			mouseMoveEvent(
					QGraphicsSceneMouseEvent*) override;

    QRectF			boundingrect_;
    MarkerStyle2D*		mstyle_;
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
				      QWidget*) override;

    void			setPaintInCenter(bool);

    int				type() const override
				{ return ODGraphicsType+3; }

protected:
    void			mouseMoveEvent(
					QGraphicsSceneMouseEvent*) override;
    void			hoverEnterEvent(
					QGraphicsSceneHoverEvent*) override;
    void			hoverLeaveEvent(
					QGraphicsSceneHoverEvent*) override;

    bool			paintincenter_		= false;
};


class ODGraphicsArrowItem : public QAbstractGraphicsShapeItem
{
public:
				ODGraphicsArrowItem();

    QRectF			boundingRect() const override;
    void			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*) override;

    void			drawArrow(QPainter&);
    double			getAddedAngle(double,float);
    QPoint			getEndPoint(const QPoint&,double,double);
    void			drawArrowHead(QPainter&,const QPoint&,
					      const QPoint&);
    void			setArrowStyle( const ArrowStyle& arrowstyle )
				{ arrowstyle_ = arrowstyle ; }
    void			setArrowSize( int arrowsz )
				{ arrowsz_ = arrowsz ; }
    void			setLineStyle(QPainter&,const OD::LineStyle&);

    int				type() const override
				{ return ODGraphicsType+4; }

protected:
    void			mouseMoveEvent(
					QGraphicsSceneMouseEvent*) override;

    ArrowStyle			arrowstyle_;
    int				arrowsz_		= 1;
};


class ODGraphicsTextItem : public QGraphicsTextItem
{
public:
			ODGraphicsTextItem();

    void		setCentered();
    void		setAlignment( const Alignment& al )	{ al_ = al; }
    Alignment		getAlignment() const			{ return al_; }
    void		overrulePaint( bool yn )	{ ownpaint_ = yn; }

    QRectF		boundingRect() const override;

protected:
    void		mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    void		contextMenuEvent(
				QGraphicsSceneContextMenuEvent*) override;

    void		paint(QPainter*,const QStyleOptionGraphicsItem*,
				QWidget*) override;
    Alignment		al_;

    bool		ownpaint_		= false;
};


class ODGraphicsAdvancedTextItem : public QGraphicsTextItem
{
public:
			ODGraphicsAdvancedTextItem(bool);

protected:
    void		mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    void		contextMenuEvent(
				QGraphicsSceneContextMenuEvent*) override;
};


class ODGraphicsPathItem : public QGraphicsPathItem
			 , public ODGraphicsHighlightItem
{
public:
				ODGraphicsPathItem();
				~ODGraphicsPathItem();

    void			set(const QPainterPath&);
    QPainterPath		shape() const override;

    void			setQPen(const QPen& pen) override;
    void			highlight() override;
    void			unHighlight() override;

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

    QRectF			boundingRect() const override;
    void			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*) override;
    QPainterPath		shape() const override;

    void			setPolyLine(const QPolygonF&,bool closed);
    void			setFillRule(Qt::FillRule);
    bool			isEmpty() const;
    void			setEmpty();

    void			setQPen(const QPen&) override;
    void			highlight() override;
    void			unHighlight() override;

    int				type() const override
				{ return ODGraphicsType+6; }

protected:
    void			mouseMoveEvent(
					QGraphicsSceneMouseEvent*) override;

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

    QRectF			boundingRect() const override;
    void			paint(QPainter*,
				      const QStyleOptionGraphicsItem*,
				      QWidget*) override;
    QPainterPath		shape() const override;

    void			setPolyLine(const QPolygonF&);
    void			setQPens(const QVector<QPen>&);
    void			setPenWidth(int);

    void			highlight() override	{ highlight_ = true; }
    void			unHighlight() override	{ highlight_ = false; }

    int				type() const override
				{ return ODGraphicsType+8; }

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

    void			mouseMoveEvent(
					QGraphicsSceneMouseEvent*) override;
    void			cleanupPolygon();

    bool			highlight_		= false;
    QPolygonF			inputqpolygon_;
    QVector<QPen>		inputqpens_;
    QVector<ODLineSegment>	odlinesegments_;
    QRectF			brect_;
    QPainterPath		path_;
};


class ODGraphicsItemGroup : public QGraphicsItemGroup
{
public:
			ODGraphicsItemGroup();

    QRectF		boundingRect() const override;
    void		paint(QPainter*,const QStyleOptionGraphicsItem*,
				QWidget*) override;
protected:
    void		mouseMoveEvent(QGraphicsSceneMouseEvent*) override;

};


class ODGraphicsDynamicImageItem : public QGraphicsItem, public CallBacker
{
public:
				ODGraphicsDynamicImageItem();

    QRectF			boundingRect() const override { return bbox_; }
    void			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*) override;

    void			setImage(bool isdynamic,const QImage&,
					 const QRectF&);
    bool			updateResolution(const QPainter*);
    const QRectF&		wantedWorldRect() const;
    const QSize&		wantedScreenSize() const;

    int				type() const override
				{ return ODGraphicsType+7; }

    Notifier<ODGraphicsDynamicImageItem>	wantsData;
    bool					isSnapshot() const;
						/*!<If set during a wantsData
						   trigger, the delivered image
						   must be of same size as
						   requested image. */

protected:
    void			mouseMoveEvent(
					QGraphicsSceneMouseEvent*) override;

    QRectF			wantedwr_;
    QSize			wantedscreensz_;

    QMutex			imagelock_;
    QWaitCondition		imagecond_;
    bool			updatedynpixmap_	= false;
    QImage			dynamicimage_;
    QRectF			dynamicimagebbox_;
    bool			dynamicrev_[2];
    bool			updatebasepixmap_	= false;
    QImage			baseimage_;
    QRectF			bbox_;
    bool			baserev_[2];
    bool			issnapshot_		= false;

    PtrMan<QPixmap>		basepixmap_;	//Only access in paint
    PtrMan<QPixmap>		dynamicpixmap_; //Only access in paint
    QRectF			dynamicpixmapbbox_; //Only access in paint

};

#endif
