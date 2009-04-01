/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		April 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicsitemimpl.cc,v 1.15 2009-04-01 14:35:39 cvsbert Exp $";

#include "uigraphicsitemimpl.h"

#include "odgraphicsitem.h"
#include "pixmap.h"
#include "polygon.h"
#include "uifont.h"

#include <QBrush>
#include <QFont>
#include <QFontMetrics>
#include <QPen>


uiEllipseItem::uiEllipseItem( const uiPoint& center, const uiSize& sz )
    : uiGraphicsItem(mkQtObj())
{
    setPos( center );
    setSize( sz );
}


uiEllipseItem::uiEllipseItem( const uiSize& size )
    : uiGraphicsItem(mkQtObj())
{
    setSize( size );
}


uiEllipseItem::uiEllipseItem()
    : uiGraphicsItem(mkQtObj())
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


void uiEllipseItem::setSize( const uiSize& size )
{
    const int width = size.hNrPics();
    const int height = size.vNrPics();
    qellipseitem_->setRect( -width/2, -height/2, width, height );
}



uiCircleItem::uiCircleItem( const uiPoint& center, int r )
    : uiEllipseItem(center,uiSize(r,r))
{}


uiCircleItem::uiCircleItem( int r )
    : uiEllipseItem(uiSize(r,r))
{}


uiLineItem::uiLineItem()
    : uiGraphicsItem(mkQtObj())
{}


uiLineItem::uiLineItem( QGraphicsLineItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qlineitem_(qtobj)
{}


uiLineItem::uiLineItem( const uiPoint& startpos, const uiPoint& endpos )
    : uiGraphicsItem(mkQtObj())
{
    setLine( startpos, endpos );
}


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


void uiLineItem::setLine( const uiPoint& from, const uiPoint& to )
{
    setLine( from.x, from.y, to.x, to.y );
}


uiRect* uiLineItem::lineRect()
{
    return new uiRect( (int)qlineitem_->line().x1(),
	    	       (int)qlineitem_->line().y1(),
	    	       (int)qlineitem_->line().x2(),
		       (int)qlineitem_->line().y2() );
}


void uiLineItem::setPenColor( const Color& col )
{
    QPen qpen = qlineitem_->pen();
    qpen.setColor( QColor(col.rgb()) );
    qlineitem_->setPen( qpen );
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


uiPixmapItem::uiPixmapItem( const ioPixmap& pixmap )
    : uiGraphicsItem(mkQtObj())
{
    setPixmap( pixmap );
}


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


uiPolygonItem::uiPolygonItem( const ODPolygon<int>& polygon, bool dofill )
    : uiGraphicsItem(mkQtObj())
{
    setPolygon( polygon );
    if ( dofill )
	fill();
}


uiPolygonItem::uiPolygonItem( const TypeSet<uiPoint>& polygon, bool dofill )
    : uiGraphicsItem(mkQtObj())
{
    setPolygon( polygon );
    if ( dofill )
	fill();
}

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
    QPolygonF qpolygonf(qpolygon);
    qpolygonitem_->setPolygon( qpolygonf );
}


void uiPolygonItem::setPolygon( const ODPolygon<int>& polygon )
{
    TypeSet<uiPoint> ptlist;
    for ( int idx=0; idx<polygon.size(); idx++ )
	ptlist += polygon.data()[idx];
    setPolygon( ptlist );
}


uiPolyLineItem::uiPolyLineItem()
    : uiGraphicsItem(mkQtObj())
{}


uiPolyLineItem::uiPolyLineItem( const TypeSet<uiPoint>& ptlist )
    : uiGraphicsItem(mkQtObj())
{
    setPolyLine( ptlist );
}


uiPolyLineItem::~uiPolyLineItem()
{
    delete qpolylineitem_;
}


void uiPolyLineItem::setPolyLine( const TypeSet<uiPoint>& ptlist )
{
    QPolygon qpolygon( ptlist.size() );
    for ( int idx=0; idx<ptlist.size(); idx++ )
	qpolygon.setPoint( (unsigned int)idx, ptlist[idx].x, ptlist[idx].y );
    qpolylineitem_->setPolyLine( qpolygon );
}


QGraphicsItem* uiPolyLineItem::mkQtObj()
{
    qpolylineitem_ = new ODGraphicsPolyLineItem();
    return qpolylineitem_;
}


uiRectItem::uiRectItem()
    : uiGraphicsItem(mkQtObj())
{}


uiRectItem::uiRectItem( QGraphicsRectItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qrectitem_(qtobj)
{}


uiRectItem::uiRectItem( int x, int y, int width, int height )
    : uiGraphicsItem(mkQtObj())
{
    setRect( x, y, width, height );
}


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
    qrectitem_->setRect( 0, 0, width, height );
    qrectitem_->setPos( x, y );
}


uiTextItem::uiTextItem()
    : uiGraphicsItem(mkQtObj())
{}


uiTextItem::uiTextItem( QGraphicsTextItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qtextitem_( qtobj )
{}


uiTextItem::uiTextItem( int x, int y, const char* txt, const Alignment& al )
    : uiGraphicsItem(mkQtObj())
{
    setText( txt );
    setPos( x, y );
    setAlignment( al );
}


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
    QFontMetrics qfm( qtextitem_->font() );
    const float txtwidth = qfm.width( qtextitem_->toPlainText() );
    const float txtheight = qfm.height();
    float movex = 0, movey = 0;
    switch ( al.hPos() )
    {
	case Alignment::Right:
	    movex = -txtwidth;
	    break;
	case Alignment::HCenter:
	    movex = -txtwidth/2;
	    break;
	case Alignment::Left:
	    break;
    }
    
    switch ( al.vPos() )
    {
	case Alignment::Bottom:
	    movey = -txtheight;
	    break;
	case Alignment::VCenter:
	    movey = -txtheight/2;
	    break;
	case Alignment::Top:
	    break;
    }

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


void uiMarkerItem::setFill( bool fill )
{
    qmarkeritem_->setFill( fill );
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


void uiArrowItem::setArrowSize( int arrowsz )
{
    qarrowitem_->setArrowSize( arrowsz );
}
