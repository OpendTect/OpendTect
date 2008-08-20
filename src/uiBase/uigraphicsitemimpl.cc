/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		April 2008
 RCS:		$Id: uigraphicsitemimpl.cc,v 1.1 2008-08-20 03:40:27 cvssatyaki Exp $
________________________________________________________________________

-*/
#include "uigraphicsitemimpl.h"

#include "draw.h"
#include "geometry.h"
#include "color.h"
#include "math.h"
#include "pixmap.h"
#include "odgraphicsitem.h"
#include "uifont.h"
#include "uigeom.h"

#include "qgraphicsitem.h"
#include "qglobal.h"
#include "qstyleoption.h"
#include "qpoint.h"
#include "qpainter.h"

#include <QRectF>
#include <QPointF>
#include <QPen>
#include <QRgb>
#include <QBrush>
#include <QString>
#include <QColor>

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


uiPixmapItem::uiPixmapItem( QGraphicsPixmapItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qpixmapitem_(qtobj)
{}


uiPixmapItem::~uiPixmapItem()
{
    delete qpixmapitem_;
}


QGraphicsItem* uiPixmapItem::mkQtObj()
{
    qpixmapitem_ = new QGraphicsPixmapItem();
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
    QString htmltext;
    htmltext += "<p style='align: ";
    switch ( al.hor_ )
    {
	case Alignment::Start:
	    htmltext += "left";
	    break;
	case Alignment::Middle:
	    htmltext += "center";
	    break;
	case Alignment::Stop:
	    htmltext += "right";
	    break;
    }

    htmltext += ";'>";
    htmltext += "<";
    htmltext += qtextitem_->toHtml();
    htmltext += ">";
    qtextitem_->setHtml( htmltext.toAscii().data() );
}


uiMarkerItem::uiMarkerItem()
    : uiGraphicsItem(mkQtObj())
{}


uiMarkerItem::uiMarkerItem( ODGraphicsMarkerItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qmarkeritem_(qtobj)
{}


uiMarkerItem::~uiMarkerItem()
{
    delete qmarkeritem_;
}


QGraphicsItem* uiMarkerItem::mkQtObj()
{
    qmarkeritem_ = new ODGraphicsMarkerItem();
    return qmarkeritem_;
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
