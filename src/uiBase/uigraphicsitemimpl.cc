/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		April 2008
 RCS:		$Id: uigraphicsitemimpl.cc,v 1.4 2008-10-09 06:39:23 cvsnanne Exp $
________________________________________________________________________

-*/
#include "uigraphicsitemimpl.h"

#include "odgraphicsitem.h"
#include "pixmap.h"
#include "uifont.h"

#include <QBrush>
#include <QFont>
#include <QFontMetrics>
#include <QPen>


uiEllipseItem::uiEllipseItem()
    : uiGraphicsItem(mkQtObj())
{}


uiEllipseItem::uiEllipseItem( QGraphicsEllipseItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qellipseitem_(qtobj)
{}


uiEllipseItem::~uiEllipseItem()
{
    delete qellipseitem_;
}


QGraphicsItem* uiEllipseItem::mkQtObj()
{
    qellipseitem_ = new QGraphicsEllipseItem();
    return qellipseitem_;
}


void uiEllipseItem::setRect( const uiRect& rect )
{
    qellipseitem_->setRect( -rect.width()/2, -rect.height()/2, rect.width(),
	    		    rect.height() );
}


uiLineItem::uiLineItem()
    : uiGraphicsItem(mkQtObj())
{}


uiLineItem::uiLineItem( QGraphicsLineItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qlineitem_(qtobj)
{}


uiLineItem::~uiLineItem()
{
    delete qlineitem_;
}


QGraphicsItem* uiLineItem::mkQtObj()
{
    qlineitem_ = new QGraphicsLineItem();
    return qlineitem_;
}


void uiLineItem::setLine( float x1, float y1, float x2, float y2 )
{
    qlineitem_->setLine( x1, y1, x2, y2 );
}

uiRect* uiLineItem::lineRect()
{
    return new uiRect( (int)qlineitem_->line().x1(),(int)qlineitem_->line().y1(),
	    	       (int)qlineitem_->line().x2(),(int)qlineitem_->line().y2() );
}


void uiLineItem::setPenColor( const Color& col )
{
    qlineitem_->pen().setColor( QColor(QRgb(col.rgb())) );
}


void uiLineItem::setPenStyle( const LineStyle& ls )
{
    QBrush qbrush( QColor(QRgb(ls.color_.rgb())) );
    QPen qpen( qbrush, ls.width_, (Qt::PenStyle)ls.type_ );
    qlineitem_->setPen( qpen );
}

// +++++ uiPixmapItem +++++

uiPixmapItem::uiPixmapItem()
    : uiGraphicsItem(mkQtObj())
{}


uiPixmapItem::uiPixmapItem( ODGraphicsPixmapItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qpixmapitem_(qtobj)
{}


uiPixmapItem::~uiPixmapItem()
{
    delete qpixmapitem_;
}


QGraphicsItem* uiPixmapItem::mkQtObj()
{
    qpixmapitem_ = new ODGraphicsPixmapItem();
    return qpixmapitem_;
}


void uiPixmapItem::setOffset( int left, int top )
{
    qpixmapitem_->setOffset( QPointF (QPoint(left,top)) );
}


void uiPixmapItem::setPixmap( const ioPixmap& pixmap )
{
    qpixmapitem_->setPixmap( *pixmap.qpixmap());
}


const uiPoint& uiPixmapItem::transformToScene( float x, float y )
{
    return *new uiPoint( (int)qpixmapitem_->mapToScene(x,y).x(),
	    		(int)qpixmapitem_->mapToScene(x,y).y() );
}


uiPolygonItem::uiPolygonItem( QGraphicsPolygonItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qpolygonitem_(qtobj)
{}


uiPolygonItem::uiPolygonItem()
    : uiGraphicsItem(mkQtObj())
{}


uiPolygonItem::~uiPolygonItem()
{
    delete qpolygonitem_;
}


QGraphicsItem* uiPolygonItem::mkQtObj()
{
    qpolygonitem_ = new QGraphicsPolygonItem();
    return qpolygonitem_;
}


void uiPolygonItem::fill()
{
    qpolygonitem_->setFillRule( Qt::OddEvenFill );
}


void uiPolygonItem::setPolygon( const TypeSet<uiPoint>& ptlist )
{
    QPolygon qpolygon( ptlist.size() );
    for ( int idx=0; idx<ptlist.size(); idx++ )
	qpolygon.setPoint( (unsigned int)idx, ptlist[idx].x, ptlist[idx].y );
    qpolygonitem_->setPolygon( QPolygonF(qpolygon) );
}


uiRectItem::uiRectItem()
    : uiGraphicsItem(mkQtObj())
{}


uiRectItem::uiRectItem( QGraphicsRectItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qrectitem_(qtobj)
{}


uiRectItem::~uiRectItem()
{
    delete qrectitem_;
}


QGraphicsItem* uiRectItem::mkQtObj()
{
    qrectitem_ = new QGraphicsRectItem();
    return qrectitem_;
}


void uiRectItem::setRect( int x, int y, int width, int height )
{
    qrectitem_->setRect( x, y, width, height );
}


uiTextItem::uiTextItem()
    : uiGraphicsItem(mkQtObj())
{}


uiTextItem::uiTextItem( QGraphicsTextItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qtextitem_( qtobj )
{}


uiTextItem::~uiTextItem()
{
    delete qtextitem_;
}


QGraphicsItem* uiTextItem::mkQtObj()
{
    qtextitem_ = new QGraphicsTextItem();
    return qtextitem_;
}

const uiRect* uiTextItem::getTextRect()
{
    QRect rect( qtextitem_->boundingRect().toRect().topLeft(),
	    	qtextitem_->boundingRect().toRect().bottomRight() );
    return new uiRect( rect.topLeft().x(), rect.topLeft().y(),
	    	       rect.bottomRight().x(), rect.bottomRight().y() );
}


void uiTextItem::setText( const char* txt )
{
    qtextitem_->setPlainText( QString(txt) );
}


void uiTextItem::setFont( const uiFont& font )
{
    qtextitem_->setFont( font.qFont() );
}


int uiTextItem::getTextWidth()
{
    return (int)qtextitem_->textWidth();
}


void uiTextItem::setAlignment( const Alignment& al )
{
    const uiRect* textrect = getTextRect();
    QFontMetrics qfm( qtextitem_->font() );
    float movex = textrect->width();//qfm.width( qtextitem_->toPlainText() );
    float movey = textrect->height();
    /*
    switch ( al.hor_ )
    {
	case Alignment::Right:
	    movex = -movex;
	    break;
	case Alignment::Middle:
	    movex = -movex/2;
	    break;
	case Alignment::Left:
	    break;
    }
    
    switch ( al.ver_ )
    {
	case Alignment::Bottom:
	    movey = -(float)qfm.height();
	    break;
	case Alignment::Middle:
	    movey = -(float)qfm.height()/2;
	    break;
	case Alignment::Top:
	    break;
    }
    */
    qtextitem_->moveBy( movex, movey );
}


void uiTextItem::setTextColor( const Color& col )
{
    qtextitem_->setDefaultTextColor( QColor(QRgb(col.rgb())) );
}

    
uiMarkerItem::uiMarkerItem( ODGraphicsMarkerItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qmarkeritem_(qtobj)
{}


uiMarkerItem::uiMarkerItem( const MarkerStyle2D& mstyle )
    : uiGraphicsItem( mkQtObj() )
{
    qmarkeritem_->setMarkerStyle( mstyle );
}


QGraphicsItem* uiMarkerItem::mkQtObj()
{
    qmarkeritem_ = new ODGraphicsMarkerItem();
    return qmarkeritem_;
}


uiMarkerItem::~uiMarkerItem()
{
    delete qmarkeritem_;
}


void uiMarkerItem::setMarkerStyle( const MarkerStyle2D& mstyle )
{
    qmarkeritem_->setMarkerStyle( mstyle );
}


uiPointItem::uiPointItem()
    : uiGraphicsItem(mkQtObj())
{}


uiPointItem::uiPointItem( ODGraphicsPointItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qpointitem_(qtobj)
{}


uiPointItem::~uiPointItem()
{
    delete qpointitem_;
}


QGraphicsItem* uiPointItem::mkQtObj()
{
    qpointitem_ = new ODGraphicsPointItem();
    return qpointitem_;
}


uiArrowItem::uiArrowItem()
    : uiGraphicsItem(mkQtObj())
{}


uiArrowItem::uiArrowItem( ODGraphicsArrowItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qarrowitem_(qtobj)
{}


uiArrowItem::~uiArrowItem()
{
    delete qarrowitem_;
}


QGraphicsItem* uiArrowItem::mkQtObj()
{
    qarrowitem_ = new ODGraphicsArrowItem();
    return qarrowitem_;
}


void uiArrowItem::setArrowStyle( const ArrowStyle& arrowstyle )
{
    qarrowitem_->setArrowStyle( arrowstyle );
}


void uiArrowItem::setArrowSize( const int arrowsz )
{
    qarrowitem_->setArrowSize( arrowsz );
}
