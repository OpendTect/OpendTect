/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		July 2008
________________________________________________________________________

-*/

#include "odgraphicsitem.h"

#include "enums.h"
#include "envvars.h"
#include "geometry.h"
#include "uifont.h"
#include "uimain.h"
#include "uipixmap.h"

#include <QColor>
#include <QGraphicsScene>
#include <QMetaObject>
#include <QPainter>
#include <QPdfWriter>
#include <QPen>
#include <QPoint>
#include <QPrinter>
#include <QRectF>
#include <QRgb>
#include <QStyleOption>
#include <QTextDocument>


mUseQtnamespace

#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    #define mGetTextWidth(qfm,textstring) qfm.horizontalAdvance( textstring )
#else
    #define mGetTextWidth(qfm,textstring) qfm.width( textstring )
#endif

static void snapToSceneRect( QGraphicsItem* itm )
{
    if ( itm->x()<0 )
	itm->setPos( 0, itm->y() );
    else if ( itm->x()+itm->boundingRect().right() > itm->scene()->width() )
	itm->setPos( itm->scene()->width()-itm->boundingRect().width(),
		     itm->y() );

    if ( itm->y()<0 )
	itm->setPos( itm->x(), 0 );
    else if ( itm->y()+itm->boundingRect().bottom() > itm->scene()->height() )
	itm->setPos( itm->x(),
		     itm->scene()->height()-itm->boundingRect().height() );
}

// ODGraphicsPointItem
ODGraphicsPointItem::ODGraphicsPointItem()
    : QAbstractGraphicsShapeItem()
    , highlight_(false)
    , penwidth_(2)
    , pencolor_(Color::Black())
{}


ODGraphicsPointItem::~ODGraphicsPointItem()
{}


QRectF ODGraphicsPointItem::boundingRect() const
{
    return highlight_ ? QRectF( -2, -2, 4, 4 )
		      : QRectF( -1,-1, 2, 2 );
}


void ODGraphicsPointItem::paint( QPainter* painter,
				 const QStyleOptionGraphicsItem* option,
				 QWidget* widget )
{
    painter->setPen( pen() );
    drawPoint( painter );

    if ( option->state & QStyle::State_Selected )
    {
	painter->setPen( QPen(option->palette.text(),1.0,
					Qt::DashLine) );
	painter->setBrush( Qt::NoBrush );
	painter->drawRect( boundingRect().adjusted(2,2,-2,-2) );
    }
}


void ODGraphicsPointItem::drawPoint( QPainter* painter )
{
    painter->setPen( pen() );
    QPoint pts[13]; int ptnr = 0;
    #define mSetPt(ox,oy) pts[ptnr].setX(ox); pts[ptnr].setY(oy); ptnr++;
    mSetPt( 0, 0 );
    mSetPt( -1, 0 ); mSetPt( 1, 0 );
    mSetPt( 0, -1 ); mSetPt( 0, 1 );
    if ( highlight_ )
    {
	mSetPt( -1, -1 ); mSetPt( 1, -1 );
	mSetPt( -1, 1 ); mSetPt( 1, 1 );
	mSetPt( 2, 0 ); mSetPt( -2, 0 );
	mSetPt( 0, 2 ); mSetPt( 0, -2 );
    }

    for ( int idx=0; idx<13; idx++ )
	painter->drawPoint( pts[idx] );
}


void ODGraphicsPointItem::mouseMoveEvent( QGraphicsSceneMouseEvent* event )
{
    QGraphicsItem::mouseMoveEvent( event );

    snapToSceneRect ( this );
}


// ODGraphicsMarkerItem
ODGraphicsMarkerItem::ODGraphicsMarkerItem()
    : QAbstractGraphicsShapeItem()
    , mstyle_( new MarkerStyle2D() )
    , fill_(false)
{
    setFlag( QGraphicsItem::ItemIgnoresTransformations, true );
}


ODGraphicsMarkerItem::~ODGraphicsMarkerItem()
{ delete mstyle_; }


void ODGraphicsMarkerItem::setMarkerStyle( const MarkerStyle2D& mstyle )
{
    const char* typestr = MarkerStyle2D::getTypeString( mstyle.type_ );
    if ( mstyle.isVisible() || mstyle.size_ != 0 || !typestr || !*typestr )
	*mstyle_ = mstyle;
}


QRectF ODGraphicsMarkerItem::boundingRect() const
{
    return QRectF( -mstyle_->size_, -mstyle_->size_,
			     2*mstyle_->size_, 2*mstyle_->size_ );
}


void ODGraphicsMarkerItem::paint( QPainter* painter,
			       const QStyleOptionGraphicsItem* option,
			       QWidget* widget )
{
   /* if ( side_ != 0 )
    pErrMsg( "TODO: implement single-sided markers" );
    if ( !mIsZero(angle_,1e-3) )
    pErrMsg( "TODO: implement tilted markers" );*/

    painter->setPen( pen() );
    if ( fill_ )
	painter->setBrush( QColor(QRgb(fillcolor_.rgb())) );

    drawMarker( *painter, mstyle_->type_, mstyle_->size_, mstyle_->size_ );

    if ( option->state & QStyle::State_Selected )
    {
	painter->setPen( QPen(option->palette.text(),1.0,
					Qt::DashLine) );
	painter->setBrush( Qt::NoBrush );
	painter->drawRect( boundingRect().adjusted(2,2,-2,-2) );
    }
}


void ODGraphicsMarkerItem::drawMarker( QPainter& painter,
		    MarkerStyle2D::Type typ, float szx, float szy )
{
    switch ( typ )
    {
	case MarkerStyle2D::Square:
	    painter.drawRect( QRectF(-szx, -szy, 2*szx, 2*szy) );
	    break;

	case MarkerStyle2D::Target:
	    szx /=2;
	    szy /=2;
	case MarkerStyle2D::Circle:
	    painter.drawEllipse( QRectF( -szx, -szy, 2*szx, 2*szy) );
	    break;

	case MarkerStyle2D::Cross:
	    painter.drawLine( QLineF(-szx, -szy, +szx, +szy) );
	    painter.drawLine( QLineF(-szx, +szy, +szx, -szy) );
	    break;

	case MarkerStyle2D::HLine:
	    painter.drawLine( QLineF( -szx, 0, +szx, 0 ) );
	    break;

	case MarkerStyle2D::VLine:
	    painter.drawLine( QLineF( 0, -szy, 0, +szy ) );
	    break;

	case MarkerStyle2D::Plus:
	    drawMarker( painter, MarkerStyle2D::HLine, szx, szy );
	    drawMarker( painter, MarkerStyle2D::VLine, szx, szy );
	    break;

	case MarkerStyle2D::Plane:
	    painter.drawRect( QRectF(-3*szx, -szy/2, 6*szx, szy) );
	    break;

	case MarkerStyle2D::Triangle: {
	    QPolygonF triangle;
	    triangle += QPointF( -szx, 0 );
	    triangle += QPointF( 0, -2*szy );
	    triangle += QPointF( +szx, 0 );
	    painter.drawPolygon( triangle );
	    } break;

	case MarkerStyle2D::Arrow:
	    drawMarker( painter, MarkerStyle2D::VLine, 2*szx, 2*szy );
	    drawMarker( painter, MarkerStyle2D::Triangle, -szx, -szy );
	    break;
	case MarkerStyle2D::None:
	    break;
    }
}


void ODGraphicsMarkerItem::mouseMoveEvent( QGraphicsSceneMouseEvent* event )
{
    QGraphicsItem::mouseMoveEvent( event );

    snapToSceneRect ( this );
}



// ODGraphicsArrowItem
ODGraphicsArrowItem::ODGraphicsArrowItem()
    : QAbstractGraphicsShapeItem()
{
}


QRectF ODGraphicsArrowItem::boundingRect() const
{
    return QRectF( -arrowsz_, -arrowsz_/2, arrowsz_, arrowsz_ );
}


void ODGraphicsArrowItem::paint( QPainter* painter,
			       const QStyleOptionGraphicsItem* option,
			       QWidget* widget )
{
    painter->setClipRect( option->exposedRect );
    painter->setPen( pen() );
    drawArrow( *painter );

    if (option->state & QStyle::State_Selected)
    {
	painter->setPen( QPen(option->palette.text(),1.0,
					Qt::DashLine) );
	painter->setBrush( Qt::NoBrush );
	painter->drawRect( boundingRect().adjusted(2,2,-2,-2) );
    }
}


void ODGraphicsArrowItem::drawArrow( QPainter& painter )
{
    setLineStyle( painter, arrowstyle_.linestyle_ );

    QPoint qpointtail( -arrowsz_, 0 );
    QPoint qpointhead( 0, 0 );
    painter.drawLine( qpointtail.x(), qpointtail.y(), qpointhead.x(),
		      qpointhead.y() );
    if ( arrowstyle_.hasHead() )
	drawArrowHead( painter, qpointhead, qpointtail );
    if ( arrowstyle_.hasTail() )
	drawArrowHead( painter, qpointtail, qpointhead );
}


void ODGraphicsArrowItem::setLineStyle( QPainter& painter,
					const OD::LineStyle& ls )
{
    pen().setStyle( (Qt::PenStyle)ls.type_ );
    pen().setColor( QColor(QRgb(ls.color_.rgb())) );
    pen().setWidth( ls.width_ );

    painter.setPen( pen() );
}


void ODGraphicsArrowItem::drawArrowHead( QPainter& painter,
					 const QPoint& qpt,
					 const QPoint& comingfrom )
{
    const float headangfac = .82; // bigger => lines closer to main line

    // In UI, Y is positive downward
    const QPoint relvec( qpt.x() - comingfrom.x(),
				   comingfrom.y() - qpt.y() );
    const double ang( Math::Atan2((double)relvec.y(),(double)relvec.x()) );

    const ArrowHeadStyle& headstyle = arrowstyle_.headstyle_;
    if ( headstyle.handedness_ == ArrowHeadStyle::TwoHanded )
    {
	switch ( headstyle.type_ )
	{
	    case ArrowHeadStyle::Square:
	    {
		TypeSet<QPoint> polypts;
		polypts += qpt;
		const QPoint pt1=getEndPoint(qpt,M_PI,headstyle.sz_);
		const QPoint pt2 = getEndPoint(qpt, -(M_PI),headstyle.sz_);
		polypts += pt1;
		polypts += pt2;
		painter.drawPolygon( polypts.arr(), 3 );
		break;
	    }
	    case ArrowHeadStyle::Cross:
	    {
		painter.drawLine( qpt, QPoint(getEndPoint(qpt,
				  getAddedAngle(ang,.25),headstyle.sz_/2)) );
		painter.drawLine( qpt, QPoint(getEndPoint(qpt,
				  getAddedAngle(ang,.75),headstyle.sz_/2)) );
		painter.drawLine( qpt, QPoint(getEndPoint(qpt,
				  getAddedAngle(ang,-.25),headstyle.sz_/2)) );
		painter.drawLine( qpt, QPoint(getEndPoint(qpt,
				  getAddedAngle(ang,-.75),headstyle.sz_/2)) );
		break;
	    }
	    case ArrowHeadStyle::Triangle:
	    case ArrowHeadStyle::Line:
	    {
		const QPoint rightend = getEndPoint( qpt,
		    getAddedAngle( ang,headangfac), headstyle.sz_ );
		const QPoint leftend = getEndPoint( qpt,
		    getAddedAngle( ang,-headangfac), headstyle.sz_ );
		painter.drawLine( qpt, rightend );
		painter.drawLine( qpt, leftend );
		if ( headstyle.type_ == ArrowHeadStyle::Triangle )
		    painter.drawLine( leftend, rightend );
		break;
	    }
	}
    }
}


double ODGraphicsArrowItem::getAddedAngle( double ang, float ratiopi )
{
    ang += ratiopi * M_PI;
    while ( ang < -M_PI ) ang += M_2PI;
    while ( ang > M_PI ) ang -= M_2PI;
    return ang;
}


QPoint ODGraphicsArrowItem::getEndPoint( const QPoint& pt,
					 double angle, double len )
{
    QPoint endpt( pt.x(), pt.y() );
    double delta = len * cos( angle );
    endpt.setX( pt.x() + mNINT32(delta) );
    // In UI, Y is positive downward
    delta = -len * sin( angle );
    endpt.setY( pt.y() + mNINT32(delta) );
    return endpt;
}


void ODGraphicsArrowItem::mouseMoveEvent( QGraphicsSceneMouseEvent* event )
{
    QGraphicsItem::mouseMoveEvent( event );

    snapToSceneRect( this );
}



// ODGraphicsTextItem
ODGraphicsTextItem::ODGraphicsTextItem()
    : QGraphicsTextItem()
{
}


QRectF getTextRect( const ODGraphicsTextItem* itm )
{
    QFontMetrics qfm( itm->font() );
    QRectF textrect = qfm.boundingRect( itm->toPlainText() );
    textrect.moveTo( 0, 0 );
    const double textwidth = textrect.width();
    const double textheight = textrect.height();

    QRectF ret = textrect;
    switch ( itm->getAlignment().hPos() )
    {
	case Alignment::Left: break; // default
	case Alignment::Right: ret.translate( -textwidth, 0 ); break;
	case Alignment::HCenter: ret.translate( -textwidth/2, 0 ); break;
    }

    switch ( itm->getAlignment().vPos() )
    {
	case Alignment::Top: break; // default
	case Alignment::Bottom: ret.translate( 0, -textheight ); break;
	case Alignment::VCenter: ret.translate( 0, -textheight/2 ); break;
    }

    return ret;
}


QRectF ODGraphicsTextItem::boundingRect() const
{
    QRectF br = QGraphicsTextItem::boundingRect();
    if ( !ownpaint_ )
	return br;

    QRectF newbr = getTextRect( this );
    return newbr;
}


void ODGraphicsTextItem::paint( QPainter* painter,
				const QStyleOptionGraphicsItem* itm,
				QWidget* qw )
{
    if ( !ownpaint_ )
    {
	QGraphicsTextItem::paint( painter, itm, qw );
	return;
    }

/*
    const QTransform& transform = painter->transform();
    qreal m11 = transform.m11();    // Horizontal scaling
    qreal m12 = transform.m12();    // Vertical shearing
    qreal m13 = transform.m13();    // =0 Horizontal Projection
    qreal m21 = transform.m21();    // Horizontal shearing
    qreal m22 = transform.m22();    // vertical scaling
    qreal m23 = transform.m23();    // =0 Vertical Projection
    qreal m31 = transform.m31();    // Horizontal Position (DX)
    qreal m32 = transform.m32();    // Vertical Position (DY)
    qreal m33 = transform.m33();    // =1 Addtional Projection Factor
*/


    const QRectF br = getTextRect( this );
    painter->drawText( br, Qt::AlignCenter, toPlainText() );

    static bool showtextbb = GetEnvVarYN( "OD_SHOW_TEXT_BOX" );
    if ( showtextbb )
	painter->drawRect( br );

    painter->save();
    painter->restore();
}


void ODGraphicsTextItem::setCentered()
{
    document()->setDefaultTextOption( QTextOption(Qt::AlignCenter) );
}


void ODGraphicsTextItem::mouseMoveEvent( QGraphicsSceneMouseEvent* ev )
{
    QGraphicsTextItem::mouseMoveEvent( ev );

    snapToSceneRect( this );
}


void ODGraphicsTextItem::contextMenuEvent(
				QGraphicsSceneContextMenuEvent* ev )
{
    if ( textInteractionFlags().testFlag(Qt::TextEditorInteraction) )
	return;

    QGraphicsTextItem::contextMenuEvent( ev );
}



// ODGraphicsAdvancedTextItem
ODGraphicsAdvancedTextItem::ODGraphicsAdvancedTextItem( bool centered )
    : QGraphicsTextItem()
{
    if ( centered )
	document()->setDefaultTextOption( QTextOption(Qt::AlignCenter) );
}


void ODGraphicsAdvancedTextItem::mouseMoveEvent( QGraphicsSceneMouseEvent* ev )
{
    QGraphicsTextItem::mouseMoveEvent( ev );

    snapToSceneRect( this );
}

void ODGraphicsAdvancedTextItem::contextMenuEvent(
				QGraphicsSceneContextMenuEvent* ev )
{
    if ( textInteractionFlags().testFlag(Qt::TextEditorInteraction) )
	return;

    QGraphicsTextItem::contextMenuEvent( ev );
}


// ODGraphicsPixmapItem
ODGraphicsPixmapItem::ODGraphicsPixmapItem()
    : QGraphicsPixmapItem()
{}


ODGraphicsPixmapItem::ODGraphicsPixmapItem( const uiPixmap& pm )
    : QGraphicsPixmapItem(*pm.qpixmap())
{}


void ODGraphicsPixmapItem::setPaintInCenter( bool yn )
{
    paintincenter_ = yn;
}


void ODGraphicsPixmapItem::paint( QPainter* painter,
			       const QStyleOptionGraphicsItem* option,
			       QWidget* widget )
{
    if ( paintincenter_ )
	setOffset( -boundingRect().width()/2., -boundingRect().height()/2. );
    painter->setClipRect( option->exposedRect );
    QGraphicsPixmapItem::paint( painter, option, widget );
}


void ODGraphicsPixmapItem::mouseMoveEvent( QGraphicsSceneMouseEvent* event )
{
    QGraphicsItem::mouseMoveEvent( event );

    snapToSceneRect ( this );
}


void ODGraphicsPixmapItem::hoverEnterEvent( QGraphicsSceneHoverEvent* event )
{
    setScale( 1.1 );
    QGraphicsItem::hoverEnterEvent( event );
}


void ODGraphicsPixmapItem::hoverLeaveEvent( QGraphicsSceneHoverEvent* event )
{
    setScale( 1.0 );
    QGraphicsItem::hoverLeaveEvent( event );
}


// ODGraphicsPolyLineItem
ODGraphicsPolyLineItem::ODGraphicsPolyLineItem()
    : QAbstractGraphicsShapeItem()
    , closed_(false)
    , fillrule_(Qt::OddEvenFill)
    , mypen_(*new QPen)
{}


ODGraphicsPolyLineItem::~ODGraphicsPolyLineItem()
{
    delete &mypen_;
}


QRectF ODGraphicsPolyLineItem::boundingRect() const
{
    return qpolygon_.boundingRect();
}


void ODGraphicsPolyLineItem::paint( QPainter* painter,
			       const QStyleOptionGraphicsItem* option,
			       QWidget* widget )
{
    const QPolygonF paintpolygon = painter->worldTransform().map( qpolygon_ );

    painter->save();
    painter->resetTransform();

    painter->setPen( pen() );

    if ( closed_ )
    {
	painter->setBrush( brush() );
	painter->drawPolygon( paintpolygon, fillrule_ );
    }
    else
    {
	painter->drawPolyline( paintpolygon );
    }

    painter->restore();
}


QPainterPath ODGraphicsPolyLineItem::shape() const
{ return path_; }


void ODGraphicsPolyLineItem::setPolyLine( const QPolygonF& polygon, bool closed)
{
    prepareGeometryChange();
    qpolygon_ = polygon;
    closed_ = closed;

    const QPolygonF poly = mapFromScene( 0, 0, 5, 5 );
    QPainterPathStroker pps;
    pps.setWidth( poly.boundingRect().width() );
    QPainterPath ppath; ppath.addPolygon( polygon );
    path_ = pps.createStroke( ppath );
}


void ODGraphicsPolyLineItem::setFillRule( Qt::FillRule fr )
{ fillrule_ = fr; }


bool ODGraphicsPolyLineItem::isEmpty() const
{ return qpolygon_.isEmpty(); }


void ODGraphicsPolyLineItem::setEmpty()
{ qpolygon_.clear(); }


void ODGraphicsPolyLineItem::mouseMoveEvent( QGraphicsSceneMouseEvent* event )
{
    QGraphicsItem::mouseMoveEvent( event );

    snapToSceneRect ( this );
}


void ODGraphicsPolyLineItem::setQPen( const QPen& qpen )
{
    mypen_ = qpen;
}


void ODGraphicsPolyLineItem::highlight()
{
    QPen qpen = mypen_;
    qpen.setWidth( qpen.width() + 2 );
    setPen( qpen );
}


void ODGraphicsPolyLineItem::unHighlight()
{
    setPen( mypen_ );
}


// ODGraphicsMultiColorPolyLineItem
ODGraphicsMultiColorPolyLineItem::ODGraphicsMultiColorPolyLineItem()
    : QAbstractGraphicsShapeItem()
{}


ODGraphicsMultiColorPolyLineItem::~ODGraphicsMultiColorPolyLineItem()
{}


QRectF ODGraphicsMultiColorPolyLineItem::boundingRect() const
{
    return brect_;
}


QPainterPath ODGraphicsMultiColorPolyLineItem::shape() const
{ return path_; }


void ODGraphicsMultiColorPolyLineItem::setPolyLine( const QPolygonF& polygon )
{
    prepareGeometryChange();
    inputqpolygon_ = polygon;
    cleanupPolygon();
}


void ODGraphicsMultiColorPolyLineItem::setQPens( const QVector<QPen>& qpens )
{
    prepareGeometryChange();
    inputqpens_ = qpens;
    cleanupPolygon();
}


bool ODGraphicsMultiColorPolyLineItem::ODLineSegment::isEmpty() const
{
    return qpolygon_.isEmpty();
}


void ODGraphicsMultiColorPolyLineItem::ODLineSegment::setWidth( int w )
{
    for ( int idx=0; idx<qpens_.size(); idx++ )
	qpens_[idx].setWidth( w );
}


void ODGraphicsMultiColorPolyLineItem::ODLineSegment::add(
					const QPointF& pt, const QPen& pen )
{
    qpolygon_.append( pt );
    qpens_.append( pen );
}


void ODGraphicsMultiColorPolyLineItem::setPenWidth( int width )
{
    for ( int idx=0; idx<inputqpens_.size(); idx++ )
	inputqpens_[idx].setWidth( width );

    for ( int idx=0; idx<odlinesegments_.size(); idx++ )
	odlinesegments_[idx].setWidth( width );
}


void ODGraphicsMultiColorPolyLineItem::cleanupPolygon()
{
    odlinesegments_.clear();

    if ( inputqpens_.isEmpty() || inputqpolygon_.isEmpty() )
	return;

    if ( inputqpens_.size() != inputqpolygon_.size() )
    {
	pErrMsg("Nr pens is different from no. of points.");
	return;
    }

    // remove undefs, create segments
    odlinesegments_.append( ODLineSegment() );
    int curseg = 0;
    for ( int idx=0; idx<inputqpolygon_.size(); idx++ )
    {
	const QPointF& pt = inputqpolygon_[idx];
	if ( mIsUdf(pt.x()) || mIsUdf(pt.y()) )
	{
	    if ( odlinesegments_[curseg].isEmpty() )
		continue;

	    odlinesegments_.append( ODLineSegment() );
	    curseg++;
	    continue;
	}

	odlinesegments_[curseg].add( pt, inputqpens_[idx] );
    }

    const QPolygonF poly = mapFromScene( 0, 0, 5, 5 );
    QPainterPathStroker pps;
    pps.setWidth( poly.boundingRect().width() );
    QPainterPath ppath;
    for ( int idx=0; idx<odlinesegments_.size(); idx++ )
	ppath.addPolygon( odlinesegments_[idx].qpolygon_ );
    path_ = pps.createStroke( ppath );
    brect_ = ppath.boundingRect();
}


void ODGraphicsMultiColorPolyLineItem::paint( QPainter* painter,
				const QStyleOptionGraphicsItem*, QWidget* )
{
    QList<QPolygonF> paintpolygons;
    for ( int idx=0; idx<odlinesegments_.size(); idx++ )
	paintpolygons.append(
		painter->worldTransform().map(odlinesegments_[idx].qpolygon_) );

    painter->save();
    painter->resetTransform();

    for ( int idx=0; idx<odlinesegments_.size(); idx++ )
    {
	QVector<QPen>& qpens = odlinesegments_[idx].qpens_;
	QPolygonF& paintpolygon = paintpolygons[idx];
	for ( int pidx=1; pidx<qpens.size(); pidx++ )
	{
	    QPen qpen = qpens[pidx];
	    if ( highlight_ ) qpen.setWidth( qpen.width()+2 );

	    painter->setPen( qpen );
	    painter->drawLine( paintpolygon[pidx-1], paintpolygon[pidx] );
	}
    }

    painter->restore();
}


void ODGraphicsMultiColorPolyLineItem::mouseMoveEvent(
				QGraphicsSceneMouseEvent* event )
{
    QGraphicsItem::mouseMoveEvent( event );

    snapToSceneRect( this );
}


// ODGraphicsPathItem
ODGraphicsPathItem::ODGraphicsPathItem()
    : QGraphicsPathItem()
    , mypen_(*new QPen)
{
}


ODGraphicsPathItem::~ODGraphicsPathItem()
{
    delete &mypen_;
}


void ODGraphicsPathItem::set( const QPainterPath& ppath )
{
    setPath( ppath );

    const QPolygonF poly = mapFromScene( 0, 0, 1, 1 );
    QPainterPathStroker pps;
    pps.setWidth( poly.boundingRect().width() );
    path_ = pps.createStroke( ppath );
}


QPainterPath ODGraphicsPathItem::shape() const
{ return path_; }


void ODGraphicsPathItem::setQPen( const QPen& qpen )
{
    mypen_ = qpen;
}


void ODGraphicsPathItem::highlight()
{
    QPen qpen = mypen_;
    qpen.setWidth( qpen.width() + 2 );
    setPen( qpen );
}


void ODGraphicsPathItem::unHighlight()
{
    setPen( mypen_ );
}


// ODGraphicsItemGroup
ODGraphicsItemGroup::ODGraphicsItemGroup()
    : QGraphicsItemGroup()
{}


QRectF ODGraphicsItemGroup::boundingRect() const
{
    return QGraphicsItemGroup::boundingRect();
}


void ODGraphicsItemGroup::paint(QPainter* painter,
				const QStyleOptionGraphicsItem* option,
				QWidget* widget)
{
    QGraphicsItemGroup::paint( painter, option, widget );
}


void ODGraphicsItemGroup::mouseMoveEvent( QGraphicsSceneMouseEvent* event )
{
    QGraphicsItem::mouseMoveEvent( event );

    snapToSceneRect ( this );
}


// ODGraphicsDynamicImageItem
ODGraphicsDynamicImageItem::ODGraphicsDynamicImageItem()
    : wantsData( this )
    , bbox_( 0, 0, 1, 1 )
{
    baserev_[0] = baserev_[1] = dynamicrev_[0] = dynamicrev_[1] = false;
}


#if QT_VERSION>=0x040700
# define mImage2PixmapImpl( image, pixmap ) pixmap->convertFromImage( image )
#else
# define mImage2PixmapImpl( image, pixmap ) \
   *pixmap = QPixmap::fromImage( image, Qt::OrderedAlphaDither )
#endif


#ifdef __mac__
# define mImage2Pixmap( image, pixmap ) \
 pixmap = new QPixmap; \
 mImage2PixmapImpl( image, pixmap )
#else
# define mImage2Pixmap( image, pixmap ) \
 if ( !pixmap ) \
    pixmap = new QPixmap; \
 mImage2PixmapImpl( image, pixmap )
#endif

void ODGraphicsDynamicImageItem::setImage( bool isdynamic,
					   const QImage& image,
					   const QRectF& rect )
{
    imagelock_.lock();
    if ( isdynamic )
    {
	dynamicimage_ = image;
	dynamicimagebbox_ = rect;
	updatedynpixmap_ = true;
	dynamicrev_[0] = false;
	dynamicrev_[1] = false;
	imagecond_.wakeAll();
    }
    else
    {
	bbox_ = rect;
	baseimage_ = image;
	updatebasepixmap_ = true;
	baserev_[0] = false;
	baserev_[1] = false;
    }

    imagelock_.unlock();

    if ( isMainThreadCurrent() )
    {
	update();
    }
    else
    {
	QObject* qobj = scene();
	if ( qobj && !QMetaObject::invokeMethod( qobj, "update",
					     Qt::QueuedConnection ))
	{
	    pErrMsg("Cannot invoke method");
	}
    }
}


bool ODGraphicsDynamicImageItem::isSnapshot() const
{ return issnapshot_; }


void ODGraphicsDynamicImageItem::paint(QPainter* painter,
			      const QStyleOptionGraphicsItem* option,
			      QWidget* widget )
{
    if ( updateResolution( painter ) )
    {
	auto* qdevice = painter->device();
	mDynamicCastGet(QImage*,qimage,qdevice)
	mDynamicCastGet(QPdfWriter*,qpdfwriter,qdevice)
	mDynamicCastGet(QPrinter*,qprinter,qdevice)

	imagelock_.lock();

	issnapshot_ = qimage || qpdfwriter || qprinter;
	wantsData.trigger();

	if ( issnapshot_ )
	{
	    const QSize wantedscreensz = wantedscreensz_;
	    while ( wantedscreensz!=dynamicimage_.size() )
		imagecond_.wait( &imagelock_, 2000 );
	}

	imagelock_.unlock();
    }

    const QTransform worldtrans = painter->worldTransform();

    imagelock_.lock();

    const QPointF pix00 = worldtrans.map( QPointF(0,0) );
    const QPointF pix11 = worldtrans.map( QPointF(1,1) );

    const bool revx = pix00.x()>pix11.x();
    const bool revy = pix00.y()>pix11.y();

    const bool dynmirror = (revx!=dynamicrev_[0] || revy!=dynamicrev_[1]);

    if ( updatedynpixmap_ || dynmirror )
    {
	if ( dynamicimagebbox_.isValid() )
	{
	    if ( dynmirror )
	    {
		mImage2Pixmap( dynamicimage_.mirrored( revx, revy ),
			       dynamicpixmap_ );
		dynamicrev_[0] = revx;
		dynamicrev_[1] = revy;
	    }
	    else
	    {
		mImage2Pixmap( dynamicimage_, dynamicpixmap_ );

		dynamicrev_[0] = false;
		dynamicrev_[1] = false;
	    }
	}
	else
	{
	    dynamicpixmap_ = 0;
	}

	dynamicpixmapbbox_ = dynamicimagebbox_;
	updatedynpixmap_ = false;
    }

    bool paintbase = true;
    QRect dynamicscenerect;

    //Check if we cover everything
    if ( dynamicpixmap_ )
    {
	dynamicscenerect = worldtrans.mapRect(dynamicpixmapbbox_).toRect();
	paintbase = !dynamicscenerect.contains( painter->viewport() );
    }

    if ( paintbase )
    {
	const bool basemirror = (revx!=baserev_[0] || revy!=baserev_[1]);
	if ( !basepixmap_ || updatebasepixmap_ || basemirror )
	{
	    if ( basemirror )
	    {
		mImage2Pixmap( baseimage_.mirrored( revx, revy ), basepixmap_ );

		baserev_[0] = revx;
		baserev_[1] = revy;
	    }
	    else
	    {
		mImage2Pixmap( baseimage_, basepixmap_ );
		baserev_[0] = false;
		baserev_[1] = false;
	    }
	}

	updatebasepixmap_ = false;
    }

    imagelock_.unlock();

    painter->save();
    painter->resetTransform();

    if ( paintbase )
    {
#if QT_VERSION < 0x050000 //Qt versions before 5.0 has a bug where large images
			  //cannot be displayed properly

	if ( basepixmap_->width() < 50000 )
	{
	    const QRect scenerect = worldtrans.mapRect(bbox_).toRect();
	    painter->drawPixmap( scenerect, *basepixmap_ );
	}
#else
	 const QRect scenerect = worldtrans.mapRect(bbox_).toRect();
	 painter->drawPixmap( scenerect, *basepixmap_ );
#endif

    }

    if ( dynamicpixmap_ )
	painter->drawPixmap( dynamicscenerect, *dynamicpixmap_ );

    painter->restore();
}


bool ODGraphicsDynamicImageItem::updateResolution( const QPainter* painter )
{
    const QRectF viewport = painter->viewport();
    const QRectF projectedwr =
	painter->worldTransform().inverted().mapRect( viewport );

    const QRectF wantedwr = projectedwr.intersected( bbox_ );
    if ( !wantedwr.isValid() )
	return false;

    const QSize wantedscreensz =
	painter->worldTransform().mapRect(wantedwr).toRect().size();

    if ( wantedwr==wantedwr_ && wantedscreensz==wantedscreensz_)
	return false;

    wantedwr_ = wantedwr;
    wantedscreensz_ = wantedscreensz;
    return true;
}


const QRectF& ODGraphicsDynamicImageItem::wantedWorldRect() const
{ return wantedwr_; }


const QSize& ODGraphicsDynamicImageItem::wantedScreenSize() const
{ return wantedscreensz_; }


void ODGraphicsDynamicImageItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseMoveEvent( event );

    snapToSceneRect ( this );
}
