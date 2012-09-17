/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		July 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: odgraphicsitem.cc,v 1.23 2012/07/10 13:06:04 cvskris Exp $";

#include "odgraphicsitem.h"

#include "enums.h"
#include "geometry.h"
#include "pixmap.h"
#include "uifont.h"

#include <math.h>

#include <QColor>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QRectF>
#include <QRgb>
#include <QStyleOption>


QRectF ODGraphicsPointItem::boundingRect() const
{
    return highlight_ ? QRectF( -2, -2, 4, 4 ) : QRectF( -1,-1, 2, 2 );
}


void ODGraphicsPointItem::paint( QPainter* painter,
				 const QStyleOptionGraphicsItem* option,
				 QWidget *widget )
{
    painter->setPen( pen() );
    drawPoint( painter );

    if ( option->state & QStyle::State_Selected )
    {
	painter->setPen( QPen(option->palette.text(),1.0,Qt::DashLine) );
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



ODGraphicsMarkerItem::ODGraphicsMarkerItem()
    : QAbstractGraphicsShapeItem()
    , mstyle_( new MarkerStyle2D() )
    , fill_(false)
{}


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
    drawMarker( *painter );

    if ( option->state & QStyle::State_Selected )
    {
	painter->setPen( QPen(option->palette.text(),1.0,Qt::DashLine) );
	painter->setBrush( Qt::NoBrush );
	painter->drawRect( boundingRect().adjusted(2,2,-2,-2) );
    }
}


void ODGraphicsMarkerItem::drawMarker( QPainter& painter )
{
    if ( fill_ )
	painter.setBrush( QColor(QRgb(fillcolor_.rgb())) );
    drawMarker( painter, mstyle_->type_, mstyle_->size_ );
}


void ODGraphicsMarkerItem::drawMarker( QPainter& painter,
					MarkerStyle2D::Type typ, int sz )
{
    switch ( typ )
    {
	case MarkerStyle2D::Square:
	    painter.drawRect( QRectF(-sz, -sz, 2*sz, 2*sz) );
	break;
	
	case MarkerStyle2D::Circle:
	    painter.drawEllipse( -sz, -sz, 2*sz, 2*sz );
	break;

	case MarkerStyle2D::Cross:
	    painter.drawLine( -sz, -sz, +sz, +sz );
	    painter.drawLine( -sz, +sz, +sz, -sz );
	break;

	case MarkerStyle2D::HLine:
	    painter.drawLine( -sz, 0, +sz, 0 );
	break;

	case MarkerStyle2D::VLine:
	    painter.drawLine( 0, -sz, 0, +sz );
	break;

	case MarkerStyle2D::Target:
	    drawMarker( painter, MarkerStyle2D::Circle, sz/2 );
	case MarkerStyle2D::Plus:
	    drawMarker( painter, MarkerStyle2D::HLine, sz );
	    drawMarker( painter, MarkerStyle2D::VLine, sz );
	break;

	case MarkerStyle2D::Plane:
	    painter.drawRect( QRectF(-3*sz, -sz/2, 6*sz, sz) );
	break;

	case MarkerStyle2D::Triangle: {
	    QPolygon triangle;
	    triangle.putPoints( 0, 3, -sz, 0, 0, -2*sz, +sz, 0 );
	    painter.drawPolygon( triangle );
	    } break;

	case MarkerStyle2D::Arrow:
	    drawMarker( painter, MarkerStyle2D::VLine, 2*sz );
	    drawMarker( painter, MarkerStyle2D::Triangle, -sz );
	break;
    }
}


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
	painter->setPen( QPen(option->palette.text(),1.0,Qt::DashLine) );
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


void ODGraphicsArrowItem::setLineStyle( QPainter& painter, const LineStyle& ls )
{
    pen().setStyle( (Qt::PenStyle)ls.type_ );
    pen().setColor( QColor(QRgb(ls.color_.rgb())) );
    pen().setWidth( ls.width_ );

    painter.setPen( pen() );
}


void ODGraphicsArrowItem::drawArrowHead( QPainter& painter, const QPoint& qpt,
					 const QPoint& comingfrom )
{
    static const float headangfac = .82; // bigger => lines closer to main line

    // In UI, Y is positive downward
    const QPoint relvec( qpt.x() - comingfrom.x(), comingfrom.y() - qpt.y() );
    const double ang( atan2((double)relvec.y(),(double)relvec.x()) );

    const ArrowHeadStyle& headstyle = arrowstyle_.headstyle_;
    if ( headstyle.handedness_ == ArrowHeadStyle::TwoHanded )
    {
	switch ( headstyle.type_ )
	{
	    case ArrowHeadStyle::Square:
	    {
	        TypeSet<QPoint> polypts;
		polypts += qpt;
	        const QPoint pt1 = getEndPoint(qpt,M_PI,headstyle.sz_);
	        const QPoint pt2 = getEndPoint(qpt,-(M_PI),headstyle.sz_);
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
    while ( ang < -M_PI ) ang += 2 * M_PI;
    while ( ang > M_PI ) ang -= 2 * M_PI;
    return ang;
}


QPoint ODGraphicsArrowItem::getEndPoint( const QPoint& pt, double angle,
					 double len )
{
    QPoint endpt( pt.x(), pt.y() );
    double delta = len * cos( angle );
    endpt.setX( pt.x() + mNINT32(delta) );
    // In UI, Y is positive downward
    delta = -len * sin( angle );
    endpt.setY( pt.y() + mNINT32(delta) );
    return endpt;
}


ODGraphicsTextItem::ODGraphicsTextItem()
    : QGraphicsTextItem()
{
}


void ODGraphicsTextItem::setTextAlignment( Alignment alignment )
{
    alignoption_.setAlignment( (Qt::Alignment)alignment.uiValue() );
}


void ODGraphicsTextItem::setText( const char* txt )
{ text_ = txt; }


QRectF ODGraphicsTextItem::boundingRect() const
{
    const uiFont& uifnt = FontList().get(
				FontData::key(FontData::GraphicsSmall ) );
    QFontMetrics fm( uifnt.qFont() );
    QRectF rectf( fm.boundingRect( text_ ) );
    return rectf;
}


void ODGraphicsTextItem::paint( QPainter* painter,
				 const QStyleOptionGraphicsItem *option,
				 QWidget *widget )
{
    painter->setClipRect( option->exposedRect );
    painter->drawText( boundingRect(), text_, alignoption_ );

    if (option->state & QStyle::State_Selected)
    {
	painter->setPen(QPen(option->palette.text(), 1.0, Qt::DashLine));
	painter->setBrush(Qt::NoBrush);
	painter->drawRect(boundingRect().adjusted(2, 2, -2, -2));
    }
}



ODGraphicsPixmapItem::ODGraphicsPixmapItem()
    : QGraphicsPixmapItem()
{}


ODGraphicsPixmapItem::ODGraphicsPixmapItem( const ioPixmap& pm )
    : QGraphicsPixmapItem(*pm.qpixmap())
{}


void ODGraphicsPixmapItem::paint( QPainter* painter,
				  const QStyleOptionGraphicsItem* option,
				  QWidget* widget )
{
    painter->setClipRect( option->exposedRect );
    QGraphicsPixmapItem::paint( painter, option, widget );
}



ODGraphicsPolyLineItem::ODGraphicsPolyLineItem()
    : QAbstractGraphicsShapeItem()
{}


QRectF ODGraphicsPolyLineItem::boundingRect() const
{
    return qpolygon_.boundingRect();
}


void ODGraphicsPolyLineItem::paint( QPainter* painter,
				    const QStyleOptionGraphicsItem* option,
				    QWidget* widget )
{
    painter->setPen( pen() );
    painter->drawPolyline( qpolygon_ );
}
