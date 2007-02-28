/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: iodraw.cc,v 1.21 2007-02-28 21:15:44 cvskris Exp $
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
#ifdef USEQT3
# include <qpaintdevicemetrics.h>
# include <qpointarray.h>
#else
# include <QPolygon>
#endif

#ifdef USEQT3
# include <qpainter.h>
# define mQPainter QPainter
#else
# include <QPainter>
# define mQPainter QPainter
#endif


ioDrawTool::ioDrawTool( QPaintDevice* handle, int x_0, int y_0 )
    : qpainter_( 0 )
    , qpen_( *new QPen() )
    , freeqpainter_ ( false )
    , qpaintdev_( handle )
#ifdef USEQT3
    , qpaintdevmetr_( 0 )
#endif
    , active_( false ) 
    , x0_( x_0 )
    , y0_( y_0 )
    , font_( &uiFontList::get() )
{}


ioDrawTool::~ioDrawTool() 
{ 
    if ( qpainter_ )
       delete qpainter_;

#ifdef USEQT3
    if ( qpaintdevmetr_ )
	delete qpaintdevmetr_;
#endif
    
    delete &qpen_;
}


void ioDrawTool::drawLine( int x1, int y1, int x2, int y2 )
{
    if ( !active() && !beginDraw() ) return;
    qpainter_->drawLine( x1-x0_, y1-y0_, x2-x0_, y2-y0_ );
}


void ioDrawTool::drawLine( const uiPoint& p1, const uiPoint& p2 )
{ drawLine ( p1.x, p1.y, p2.x, p2.y ); }


void ioDrawTool::drawLine( const TypeSet<uiPoint>& pts, int pt0, int nr )
{
    if ( !active() && !beginDraw() ) return;

    int nrpoints = nr > 0 ? nr : pts.size();
#ifdef USEQT3
    QPointArray qarray( nrpoints );
#else
    QPolygon qarray( nrpoints );
#endif

    for ( int idx=0; idx<nrpoints; idx++ )
	qarray.setPoint( (unsigned int)idx, pts[idx+pt0].x, pts[idx+pt0].y);

    qpainter_->drawPolyline( qarray );
}


void ioDrawTool::drawPolygon( const TypeSet<uiPoint>& pts, int pt0, int nr )
{
    if ( !active() && !beginDraw() ) return;

    int nrpoints = nr > 0 ? nr : pts.size();
#ifdef USEQT3
    QPointArray qarray( nrpoints );
#else
    QPolygon qarray( nrpoints );
#endif

    for ( int idx=0; idx<nrpoints; idx++ )
	qarray.setPoint( (unsigned int)idx, pts[idx+pt0].x, pts[idx+pt0].y);

    qpainter_->drawPolygon( qarray );
}


void ioDrawTool::drawText( int x, int y, const char* txt, const Alignment& al, 
			   bool doovershoot, bool erase )
{
    if ( !active() && !beginDraw() ) return;

    int hgt = font_->height();
    int wdt = font_->width( txt );

    int xx = x - x0_;
    switch( al.hor )
    {
	case Alignment::Middle:
	    xx -= wdt/2;
	    break;
	case Alignment::Stop:
	    xx -= wdt;
	    break;
    }
  
    int yy = y + font_->ascent() - y0_;
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
#ifdef USEQT3
	qpainter_->fillRect( xx, yy-hgt, wdt, hgt, qpainter_->backgroundColor() );
#else
	qpainter_->eraseRect( xx, yy-hgt, wdt, hgt );
#endif

    qpainter_->drawText( xx, yy, QString(txt) );
}


void ioDrawTool::drawText( const uiPoint& p, const char* txt,
			   const Alignment& al, bool over, bool erase )
{ drawText( p.x, p.y, txt, al, over, erase ); }


void ioDrawTool::drawEllipse ( int x, int y, int w, int h )
{
    if ( !active() && !beginDraw() ) return;
    qpainter_->drawEllipse ( x-x0_, y-y0_, w, h );
}


void ioDrawTool::drawEllipse( const uiPoint& topLeft, const uiSize& sz )
{ drawEllipse( topLeft.x, topLeft.y, sz.hNrPics(), sz.vNrPics()); }


void ioDrawTool::drawEllipse( const uiRect& r )
{ drawEllipse( r.left(), r.top(), r.hNrPics(), r.vNrPics()); }


void ioDrawTool::drawRect ( int x, int y, int w, int h )
{
    if ( !active() && !beginDraw() ) return;
    const int pensize = mMAX(1,qpainter_->pen().width() );
    qpainter_->drawRect ( x-x0_, y-y0_, w-pensize, h-pensize );
}


void ioDrawTool::drawRect( const uiPoint& topLeft, const uiSize& sz )
{ drawRect( topLeft.x, topLeft.y, sz.hNrPics(), sz.vNrPics()); }


void ioDrawTool::drawRect( const uiRect& r )
{ drawRect( r.left(), r.top(), r.hNrPics(), r.vNrPics()); }


Color ioDrawTool::backgroundColor() const
{
    if ( !active() ) return Color::Black;
#ifdef USEQT3
    return Color( qpainter_->backgroundColor().rgb() );
#else
    return Color( qpainter_->background().color().rgb() );
#endif
}


void ioDrawTool::setBackgroundColor( const Color& c )
{
    if ( !active() && !beginDraw() ) return;
#ifdef USEQT3
    qpainter_->setBackgroundColor( QColor( QRgb(c.rgb()) ) );
#else
    QBrush br( qpainter_->background() );
    br.setColor( QColor( QRgb(c.rgb()) ) );
    qpainter_->setBackground( br );
#endif
}


void ioDrawTool::clear( const uiRect* rect, const Color* col )
{
    if ( !active() && !beginDraw() ) return;

    uiRect r( 0, 0, getDevWidth(), getDevHeight() );
    if ( rect ) 
    {
	r = *rect;
	if ( x0_ || y0_ )
	{
	    r.setLeft( r.left() - x0_ );
	    r.setRight( r.right()  - x0_ );
	    r.setTop( r.top()  - y0_ );
	    r.setBottom( r.bottom()- y0_ );
	}
    }

#ifndef USEQT3
    qpainter_->eraseRect( 0, 0, getDevWidth(), getDevHeight() );
    return;
#endif
    Color c( backgroundColor() );
    if ( !col ) col = &c;
    setPenColor( *col ); setFillColor( *col );
    drawRect( r );
}


void ioDrawTool::drawBackgroundPixmap( const Color* col )
{
    if ( !active() && !beginDraw() ) return;
    setBackgroundColor( *col );
    qpainter_->setBackgroundMode( Qt::OpaqueMode );
    qpainter_->setBrush( Qt::DiagCrossPattern );
    drawRect( uiRect( 0, 0, getDevWidth(), getDevHeight() ) );
}


void ioDrawTool::drawPixmap (const uiPoint& destTopLeft, const ioPixmap* pm,
			     const uiRect& srcAreaInPixmap )
{
    if ( !pm || !pm->Pixmap() ) 
    { 
	pErrMsg( "ioDrawTool::drawPixmap : No pixmap" ); 
	return; 
    }

    if ( !active() && !beginDraw() ) return;

    QRect src( QPoint(srcAreaInPixmap.left(),srcAreaInPixmap.top()),
	       QPoint(srcAreaInPixmap.right(),srcAreaInPixmap.bottom()) );
    
    QPoint dest( destTopLeft.x-x0_, destTopLeft.y-y0_ );

    qpainter_->drawPixmap( dest, *pm->Pixmap(), src );

}


void ioDrawTool::drawPixmap( int left, int top, ioPixmap* pm,
			     int sLeft, int sTop, int sRight, int sBottom )
{ drawPixmap( uiPoint( left, top ), pm, uiRect( sLeft, sTop, sRight, sBottom )); }


int ioDrawTool::getDevHeight() const
//! \return height in pixels of the device we're drawing on
{
#ifdef USEQT3
    if ( !qpaintdevmetr_ )
    { 
	if ( !qpaintdev_ )  
	{
	    pErrMsg("No paint device! Can not construct QPaintDevMetrics");
	    return 0;
	}

	const_cast<ioDrawTool*>(this)->qpaintdevmetr_
					= new QPaintDeviceMetrics( qpaintdev_ );
    }

    return qpaintdevmetr_->height();
#else
    return qpaintdev_->height();
#endif
}


int ioDrawTool::getDevWidth() const
//! \return width in pixels of the device we're drawing on
{
#ifdef USEQT3
    if ( !qpaintdevmetr_ )
    { 
	if ( !qpaintdev_ )  
	{
	    pErrMsg("No paint device! Can not construct QPaintDevMetrics");
	    return 0;
	}

	const_cast<ioDrawTool*>(this)->qpaintdevmetr_
					= new QPaintDeviceMetrics( qpaintdev_ );
    }

    return qpaintdevmetr_->width();
#else
    return qpaintdev_->width(); 
#endif
}


uiSize ioDrawTool::getDevSize() const
{ return uiSize( getDevWidth(), getDevHeight() ); }


void ioDrawTool::drawMarker( const uiPoint& pt, const MarkerStyle2D& mstyle,
			    const char* txt, bool below )
{
    setPenColor( mstyle.color );
    setFillColor( mstyle.color );
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
    qpen_.setStyle( (Qt::PenStyle) ls.type);
    qpen_.setColor( QColor( QRgb( ls.color.rgb() )));
    qpen_.setWidth( ls.width );

    if ( !active() ) return;
    qpainter_->setPen( qpen_ ); 
}


void ioDrawTool::setPenColor( const Color& colr )
{
    qpen_.setColor( QColor( QRgb(colr.rgb()) ) );
    if ( qpainter_ ) qpainter_->setPen( qpen_ ); 
}


void ioDrawTool::setFillColor( const Color& colr )
{ 
    if ( !active() && !beginDraw() ) return;

    qpainter_->setBrush( QColor( QRgb(colr.rgb()) ) );
}


void ioDrawTool::setPenWidth( unsigned int w )
{
    qpen_.setWidth( w );
    if ( !active() ) return;
    qpainter_->setPen( qpen_ ); 
}


void ioDrawTool::setFont( const uiFont& f )
{
    font_ = &f;
    if ( !active() ) return;
    qpainter_->setFont( font_->qFont() );
}

/*! start drawing
\return true if active

*/
bool ioDrawTool::beginDraw()
{
    if ( !qpaintdev_ && !qpainter_ ) return false;

    if ( !qpainter_ ) 
    {
	qpainter_ = new mQPainter;
	freeqpainter_ = true;
    }

    if ( !active_ ) 
    {
        if ( !qpainter_->isActive() )
	{
	    if ( !qpaintdev_ || !qpainter_->begin( qpaintdev_ ) )
            {
		pErrMsg("Unable to make painter active!");
		return false;
            }
	}  

	active_ = true;
    }

    qpainter_->setPen( qpen_ ); 
    qpainter_->setFont( font_->qFont() ); 

    return true;
}


bool ioDrawTool::endDraw()
{
    if ( !qpainter_ ) return false;
    
    if ( active_ )
    {   
#ifdef USEQT3
	qpainter_->flush();
#endif
	qpainter_->end();
        active_ = false;
    }

    if ( freeqpainter_ ) delete qpainter_;
    qpainter_ = 0;

    return true;
}


bool ioDrawTool::setActivePainter( mQPainter* p )
{
    if ( qpainter_ ) return false;
    if ( active_ ) return false;

    qpainter_ = p;
    freeqpainter_ = false;
    active_ = true;

    return true;
}


void ioDrawTool::setRasterXor()
{
    if ( !active() ) return;
#ifdef USEQT3
    qpainter_->setRasterOp( Qt::XorROP );
#else 
    qpainter_->setCompositionMode( mQPainter::CompositionMode_Xor );
#endif
}


void ioDrawTool::setRasterNorm()
{
    if ( !active() ) return;
#ifdef USEQT3
    qpainter_->setRasterOp( Qt::CopyROP );
#else
    qpainter_->setCompositionMode(
			    mQPainter::CompositionMode_SourceOver );
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
