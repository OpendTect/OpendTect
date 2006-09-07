/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: iodraw.cc,v 1.16 2006-09-07 15:44:24 cvskris Exp $
________________________________________________________________________

-*/

#include "iodrawtool.h"
#include "iodrawimpl.h"
#include "errh.h"
#include "pixmap.h"
#include "color.h"
#include "uifont.h"
#include "draw.h"

#include <qpainter.h>
#include <qpen.h>
#include <qbrush.h>
#ifdef USEQT4
# include <qpolygon.h>
#else
# include <qpaintdevicemetrics.h>
# include <qpointarray.h>
#endif


ioDrawTool::ioDrawTool( QPaintDevice* handle, int x_0, int y_0 )
    : qpainter( 0 )
    , qpen( *new QPen() )
    , freeqpainter ( false )
    , qpaintdev( handle )
#ifndef USEQT4
    , qpaintdevmetr( 0 )
#endif
    , active_( false ) 
    , x0( x_0 )
    , y0( y_0 )
    , font_( &uiFontList::get() )
{}


ioDrawTool::~ioDrawTool() 
{ 
    if ( qpainter )
       delete qpainter;

#ifndef USEQT4
    if ( qpaintdevmetr )
	delete qpaintdevmetr;
#endif
    
    delete &qpen;
}


void ioDrawTool::drawLine( int x1, int y1, int x2, int y2 )
{
    if ( !active() && !beginDraw() ) return;
    qpainter->drawLine( x1-x0, y1-y0, x2-x0, y2-y0 );
}


void ioDrawTool::drawLine( const TypeSet<uiPoint>& pts, int pt0, int nr )
{
    if ( !active() && !beginDraw() ) return;

    int nrpoints = nr > 0 ? nr : pts.size();
#ifdef USEQT4
    QPolygon qarray(nrpoints);
#else
    QPointArray qarray(nrpoints);
#endif

    for ( int idx=0; idx<nrpoints; idx++ )
	qarray.setPoint( (unsigned int)idx, pts[idx+pt0].x, pts[idx+pt0].y);

    qpainter->drawPolyline( qarray );
}


void ioDrawTool::drawPolygon( const TypeSet<uiPoint>& pts, int pt0, int nr )
{
    if ( !active() && !beginDraw() ) return;

    int nrpoints = nr > 0 ? nr : pts.size();
#ifdef USEQT4
    QPolygon qarray(nrpoints);
#else
    QPointArray qarray(nrpoints);
#endif

    for ( int idx=0; idx<nrpoints; idx++ )
	qarray.setPoint( (unsigned int)idx, pts[idx+pt0].x, pts[idx+pt0].y);

    qpainter->drawPolygon( qarray );
}


void ioDrawTool::drawText( int x, int y, const char* txt, const Alignment& al, 
			   bool doovershoot, bool erase )
{
    if ( !active() && !beginDraw() ) return;

    int hgt = font_->height();
    int wdt = font_->width( txt );

    int xx = x - x0;
    switch( al.hor )
    {
	case Alignment::Middle:
	    xx -= wdt/2;
	    break;
	case Alignment::Stop:
	    xx -= wdt;
	    break;
    }
  
    int yy = y + font_->ascent() - y0;
    switch( al.ver )
    {
	case Alignment::Middle:
	    yy -= hgt/2;
	    break;
	case Alignment::Stop:
	    yy -= hgt;
	    break;
    }

    int overshoot = doovershoot ? (xx+wdt) - getDevWidth() : 0;
    if ( overshoot > 0 )
    {
	if ( 4*( wdt - overshoot ) >=  wdt )
	    xx -= overshoot;
    }
    if ( xx < 0 )
    {
	if ( 4*(wdt+xx) >= wdt )
	    xx=0;
	else return;
    }

    overshoot = doovershoot ? yy - getDevHeight() : 0;
    if ( overshoot > 0 )
    {
	if ( (hgt - overshoot)*4 >= hgt )
	    yy -= overshoot;
    }
    if ( yy < hgt )
    {
	if ( yy*4 >= hgt )
	    yy = hgt;
	else return;
    }

    if ( erase )
#ifdef USEQT4
	qpainter->eraseRect( xx, yy-hgt, wdt, hgt );
#else
	qpainter->fillRect( xx, yy-hgt, wdt, hgt, qpainter->backgroundColor() );
#endif

    qpainter->drawText( xx, yy, QString(txt) );
}


void ioDrawTool::drawEllipse ( int x, int y, int w, int h )
{
    if ( !active() && !beginDraw() ) return;
    qpainter->drawEllipse ( x-x0, y-y0, w, h );
}


void ioDrawTool::drawRect ( int x, int y, int w, int h )
{
    if ( !active() && !beginDraw() ) return;
    qpainter->drawRect ( x-x0, y-y0, w, h );
}


Color ioDrawTool::backgroundColor() const
{
    if ( !active() ) return Color::Black;
#ifdef USEQT4
    return Color( qpainter->background().color().rgb() );
#else
    return Color( qpainter->backgroundColor().rgb() );
#endif
}


void ioDrawTool::setBackgroundColor( const Color& c )
{
    if ( !active() && !beginDraw() ) return;
#ifdef USEQT4
    QBrush br( qpainter->background() );
    br.setColor( QColor( QRgb( c.rgb() )));
    qpainter->setBackground( br );
#else
    qpainter->setBackgroundColor( QColor( QRgb( c.rgb() )));
#endif
}


void ioDrawTool::clear( const uiRect* rect, const Color* col )
{
    uiRect r( 0, 0, getDevWidth(), getDevHeight() );
    if ( rect ) 
    {
	r = *rect;
	if ( x0 || y0 )
	{
	    r.setLeft( r.left() - x0 );
	    r.setRight( r.right()  - x0 );
	    r.setTop( r.top()  - y0 );
	    r.setBottom( r.bottom()- y0 );
	}
    }
    Color c( backgroundColor() );
    if ( !col ) col = &c;
    setPenColor( *col ); setFillColor( *col );
    drawRect( r );
}


void ioDrawTool::drawBackgroundPixmap( const Color* col )
{
    if ( !active() && !beginDraw() ) return;
    setBackgroundColor( *col );
    qpainter->setBackgroundMode( Qt::OpaqueMode );
    qpainter->setBrush( Qt::DiagCrossPattern );
    drawRect( uiRect( 0, 0, getDevWidth(), getDevHeight() ) );
}


void ioDrawTool::drawPixmap (const uiPoint destTopLeft, ioPixmap* pm,
						 const uiRect srcAreaInPixmap )
{
    if ( !pm || !pm->Pixmap() ) 
    { 
	pErrMsg( "ioDrawTool::drawPixmap : No pixmap" ); 
	return; 
    }

    if ( !active() && !beginDraw() ) return;

    QRect src( QPoint(srcAreaInPixmap.left(),srcAreaInPixmap.top()),
	       QPoint(srcAreaInPixmap.right(),srcAreaInPixmap.bottom()) );
    
    QPoint dest( destTopLeft.x-x0, destTopLeft.y-y0 );

    qpainter->drawPixmap( dest, *pm->Pixmap(), src );

}


int ioDrawTool::getDevHeight() const
//! \return height in pixels of the device we're drawing on
{
#ifdef USEQT4
    return qpaintdev->height();
#else
    if ( !qpaintdevmetr )
    { 
	if ( !qpaintdev )  
	{
	    pErrMsg("No paint device! Can not construct QPaintDevMetrics");
	    return 0;
	}

	const_cast<ioDrawTool*>(this)->qpaintdevmetr
					= new QPaintDeviceMetrics( qpaintdev );
    }

    return qpaintdevmetr->height();
#endif
}


int ioDrawTool::getDevWidth() const
//! \return width in pixels of the device we're drawing on
{
#ifdef USEQT4
    return qpaintdev->width(); 
#else
    if ( !qpaintdevmetr )
    { 
	if ( !qpaintdev )  
	{
	    pErrMsg("No paint device! Can not construct QPaintDevMetrics");
	    return 0;
	}

	const_cast<ioDrawTool*>(this)->qpaintdevmetr
					= new QPaintDeviceMetrics( qpaintdev );
    }

    return qpaintdevmetr->width();
#endif
}


void ioDrawTool::drawMarker( uiPoint pt, const MarkerStyle2D& mstyle,
			    const char* txt, bool below )
{
    setPenColor( mstyle.color );
    switch ( mstyle.type )
    {
    case MarkerStyle2D::Square:
	drawRect( pt.x-mstyle.size, pt.y-mstyle.size,
		  2 * mstyle.size, 2 * mstyle.size );
        break;
    case MarkerStyle2D::Circle:
	drawEllipse( pt.x - mstyle.size, pt.y - mstyle.size,
		     2*mstyle.size, 2*mstyle.size );
        break;
    case MarkerStyle2D::Cross:
	drawLine( pt.x-mstyle.size, pt.y-mstyle.size,
		  pt.x+mstyle.size, pt.y+mstyle.size );
	drawLine( pt.x-mstyle.size, pt.y+mstyle.size,
		  pt.x+mstyle.size, pt.y-mstyle.size );
        break;
    }

    if ( txt && *txt )
    {
	Alignment al( Alignment::Middle, Alignment::Middle );
	int yoffs = 0;
	if ( mstyle.type != MarkerStyle2D::None )
	{
	    al.ver = below ? Alignment::Start : Alignment::Stop;
	    yoffs = below ? mstyle.size+1 : -mstyle.size-1;
	}

	drawText( uiPoint(pt.x,pt.y+yoffs), txt, al, false );
    }
}


void ioDrawTool::setLineStyle( const LineStyle& ls )
{
    qpen.setStyle( (Qt::PenStyle) ls.type);
    qpen.setColor( QColor( QRgb( ls.color.rgb() )));
    qpen.setWidth( ls.width );

    if ( !active() ) return;
    qpainter->setPen( qpen ); 
}


void ioDrawTool::setPenColor( const Color& colr )
{
    qpen.setColor( QColor( QRgb(colr.rgb()) ) );
    if ( qpainter ) qpainter->setPen( qpen ); 
}


void ioDrawTool::setFillColor( const Color& colr )
{ 
    if ( !active() && !beginDraw() ) return;

    qpainter->setBrush( QColor( QRgb(colr.rgb()) ) );
}


void ioDrawTool::setPenWidth( unsigned int w )
{
    qpen.setWidth( w );
    if ( !active() ) return;
    qpainter->setPen( qpen ); 
}


void ioDrawTool::setFont( const uiFont& f )
{
    font_ = &f;
    if ( !active() ) return;
    qpainter->setFont( font_->qFont() );
}

/*! start drawing
\return true if active

*/
bool ioDrawTool::beginDraw()
{
    if ( !qpaintdev && !qpainter ) return false;

    if ( !qpainter ) 
    {
	qpainter = new QPainter;
	freeqpainter = true;
    }

    if ( !active_ ) 
    {
        if ( !qpainter->isActive() )
	{
	    if ( !qpaintdev || !qpainter->begin( qpaintdev ) )
            {
		pErrMsg("Unable to make painter active!");
		return false;
            }
	}  

	active_ = true;
    }

    qpainter->setPen( qpen ); 
    qpainter->setFont( font_->qFont() ); 

    return true;
}


bool ioDrawTool::endDraw()
{
    if ( !qpainter ) return false;
    
    if ( active_ )
    {   
#ifndef USEQT4
	qpainter->flush();
#endif
	qpainter->end();
        active_ = false;
    }

    if ( freeqpainter ) delete qpainter;
    qpainter = 0;

    return true;
}


bool ioDrawTool::setActivePainter( QPainter* p )
{
    if ( qpainter ) return false;
    if ( active_ ) return false;

    qpainter = p;
    freeqpainter = false;
    active_ = true;

    return true;
}


void ioDrawTool::setRasterXor()
{
    if ( !active() ) return;
#ifdef USEQT4
    qpainter->setCompositionMode( QPainter::CompositionMode_Xor );
#else 
    qpainter->setRasterOp(Qt::XorROP);
#endif
}

void ioDrawTool::setRasterNorm()
{
    if ( !active() ) return;
#ifdef USEQT4
    qpainter->setCompositionMode(
			    QPainter::QPainter::CompositionMode_SourceOver );
#else
    qpainter->setRasterOp(Qt::CopyROP);
#endif
}


ioDrawAreaImpl::~ioDrawAreaImpl()
{ 
    delete drawtool;
}


ioDrawTool* ioDrawAreaImpl::drawTool_( int x0, int y0 )
{
    if ( drawtool )
	drawtool->setOrigin( x0, y0 );
    else
	drawtool = new ioDrawTool(mQPaintDevice(),x0,y0);

    return drawtool;
};


void ioDrawAreaImpl::releaseDrawTool()
{
    if ( !drawtool ) return;

    delete drawtool;
    drawtool=0;
}
