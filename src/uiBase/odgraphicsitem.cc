/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		July 2008
 RCS:		$Id: odgraphicsitem.cc,v 1.3 2008-10-09 06:35:33 cvssatyaki Exp $
________________________________________________________________________

-*/
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
#include <QString>
#include <QStyleOption>
#include <QTextOption>


QRectF ODGraphicsPointItem::boundingRect() const
{
    qreal penWidth = pen().widthF();
    return QRectF( -penWidth, -penWidth, 2*penWidth, 2*penWidth );
}


void ODGraphicsPointItem::paint( QPainter* painter,
				 const QStyleOptionGraphicsItem* option,
				 QWidget *widget )
{
    painter->setClipRect( option->exposedRect );
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
    /*QBrush qbrush( QColor(QRgb(pencolor_.rgb())) );
    QPen qpen( qbrush, penwidth_, Qt::SolidLine );
    painter->setPen( qpen );*/
    QPoint pts[13]; int ptnr = 0;
    #define mSetPt(ox,oy) pts[ptnr].setX(ox); pts[ptnr++].setY(oy)
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
{}


void ODGraphicsMarkerItem::setMarkerStyle( const MarkerStyle2D& mstyle )
{
    const char* typestr = eString( MarkerStyle2D::Type, mstyle.type_ );
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

    painter->setPen( QColor(QRgb(mstyle_->color_.rgb())) );
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
    switch ( mstyle_->type_ )
    {
	case MarkerStyle2D::Square:
	{
	    painter.drawRect( -mstyle_->size_, -mstyle_->size_,
		      	      2*mstyle_->size_, 2*mstyle_->size_ );
	    boundingrect_ = QRectF( -mstyle_->size_, -mstyle_->size_,
		    		    2*mstyle_->size_, 2*mstyle_->size_ );
	    break;
	}
	
	case MarkerStyle2D::Circle:
	{
	    painter.drawEllipse( 0, 0, mstyle_->size_ / 2, mstyle_->size_ / 2 );
	    break;
	}
	
	case MarkerStyle2D::Cross:
	{
	    painter.drawLine( -mstyle_->size_, -mstyle_->size_,
			      +mstyle_->size_, +mstyle_->size_ );
	    painter.drawLine( -mstyle_->size_, +mstyle_->size_,
			      +mstyle_->size_, -mstyle_->size_ );
	    break;
	}
    }
}


ODGraphicsArrowItem::ODGraphicsArrowItem()
    : boundingrect_(new QRectF())
{
}


QRectF ODGraphicsArrowItem::boundingRect() const
{
    qreal penWidth = pen().widthF();
    return QRectF( -penWidth, -penWidth, penWidth, penWidth );
}


void ODGraphicsArrowItem::paint( QPainter* painter,
				 const QStyleOptionGraphicsItem* option,
				 QWidget* widget )
{
    painter->setClipRect( option->exposedRect );
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

    qpointhead_->setX(0);
    qpointhead_->setY(0);
    qpointtail_->setX( arrowsz_);
    qpointtail_->setY(0);
    painter.drawLine( *qpointtail_, *qpointhead_ ); 
    if ( arrowstyle_.hasHead() )
	drawArrowHead( painter, *qpointhead_, *qpointtail_ );
    if ( arrowstyle_.hasTail() )
	drawArrowHead( painter, *qpointtail_, *qpointhead_ );
}


void ODGraphicsArrowItem::setLineStyle( QPainter& painter, const LineStyle& ls )
{
    pen().setStyle( (Qt::PenStyle)ls.type_ );
    pen().setColor( QColor(QRgb(ls.color_.rgb())) );
    pen().setWidth( ls.width_ );

    painter.setPen( pen() );
}


void ODGraphicsArrowItem::drawArrowHead( QPainter& painter, const QPoint& pos,
					 const QPoint& comingfrom )
{
    static const float headangfac = .82; // bigger => lines closer to main line

    // In UI, Y is positive downward
    const QPoint relvec( pos.x() - comingfrom.x(), comingfrom.y() - pos.y() );
    const double ang( atan2(relvec.y(),relvec.x()) );

    const ArrowHeadStyle& headstyle = arrowstyle_.headstyle_;
    if ( headstyle.handedness_ == ArrowHeadStyle::TwoHanded )
    {
	switch ( headstyle.type_ )
	{
	    case ArrowHeadStyle::Square:
	    {
	        TypeSet<QPoint> polypts;
		polypts += pos;
	        const QPoint pt1 = getEndPoint(pos,M_PI,headstyle.sz_);
	        const QPoint pt2 = getEndPoint(pos,-(M_PI),headstyle.sz_);
		boundingrect_->setRect( pt1.x(), pt1.y() , pt1.x() + arrowsz_ ,
					2*pt1.y() );
		polypts += pt1;
		polypts += pt2;
		painter.drawPolygon( polypts.arr(), 3 );
		break;
	    }
	    case ArrowHeadStyle::Cross:
	    {
		painter.drawLine( pos, QPoint(getEndPoint(pos,
				  getAddedAngle(0,.25),headstyle.sz_/2)) );
		painter.drawLine( pos, QPoint(getEndPoint(pos,
				  getAddedAngle(0,.75),headstyle.sz_/2)) );
		painter.drawLine( pos, QPoint(getEndPoint(pos,
				  getAddedAngle(0,-.25),headstyle.sz_/2)) );
		painter.drawLine( pos, QPoint(getEndPoint(pos,
				  getAddedAngle(0,-.75),headstyle.sz_/2)) );
		break;
	    }
	    case ArrowHeadStyle::Triangle:
	    case ArrowHeadStyle::Line:
	    {
		const QPoint rightend = getEndPoint( pos,
		    getAddedAngle(0,headangfac), headstyle.sz_ );
		const QPoint leftend = getEndPoint( pos,
		    getAddedAngle(0,-headangfac), headstyle.sz_ );
		painter.drawLine( pos, rightend );
		painter.drawLine( pos, leftend );
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
    endpt.setX( pt.x() + mNINT(delta) );
    // In UI, Y is positive downward
    delta = -len * sin( angle );
    endpt.setY( pt.y() + mNINT(delta) );
    return endpt;
}


ODGraphicsTextItem::ODGraphicsTextItem()
    : text_(new QString() )
    , alignoption_(new QTextOption() )
{
}


void ODGraphicsTextItem::setTextAlignment( OD::Alignment alignment )
{
    alignoption_->setAlignment( (Qt::Alignment)int(alignment) );
}


void ODGraphicsTextItem::setText( const char* txt )
{
    text_ = new QString( txt );
}


QRectF ODGraphicsTextItem::boundingRect() const
{
    const uiFont& font = uiFontList::get(
	    			FontData::key(FontData::GraphicsSmall ) );
    QFontMetrics fm( font.qFont() );
    QRectF rectf( fm.boundingRect( *text_ ) );
    return rectf;
}


void ODGraphicsTextItem::paint( QPainter* painter,
				 const QStyleOptionGraphicsItem *option,
				 QWidget *widget )
{
    painter->setClipRect( option->exposedRect );
    painter->drawText( boundingRect(), *text_, *alignoption_ );

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
