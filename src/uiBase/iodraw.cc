/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: iodraw.cc,v 1.1 2000-11-27 10:20:34 bert Exp $
________________________________________________________________________

-*/

#include "iodraw.h"
#include "errh.h"
#include "pixmap.h"
#include "color.h"
#include "uifont.h"
#include "draw.h"

#include <qnamespace.h> 
#include <qpainter.h>
#include <qpaintdevicemetrics.h> 
#include <qpen.h>
#include <qbrush.h> 

ioDrawTool::ioDrawTool( QPaintDevice* client, int x_0, int y_0 )
    : mQPainter( 0 )
    , mQPen( *new QPen() )
    , freeMQPainter ( false )
    , mQPaintDev( client )
    , mQPaintDevMetrics( 0 )
    , active_( false ) 
    , x0( x_0 )
    , y0( y_0 )
    , font_( &uiFontList::get() )
{
//    setPenColor( Color::Black );
}


ioDrawTool::~ioDrawTool() 
{ 
    if( mQPainter )
       delete mQPainter;
    
    if( mQPaintDevMetrics )
       delete mQPaintDevMetrics;

    delete &mQPen;
}

void ioDrawTool::drawLine ( int x1, int y1, int x2, int y2 )
{
    if( !active() && !beginDraw()) return;
    mQPainter->drawLine ( x1 - x0, y1 - y0, x2 - x0, y2 - y0 );
} 

void ioDrawTool::drawText ( int x, int y, const char* txt, Alignment al, 
			    bool erase, int len )
{
    if( !active() && !beginDraw()) return;

    int hgt = font_->height();
    int wdt = font_->width( txt );

    int xx = x - x0;
    switch( al.hor ){
	case Alignment::Middle:
	    xx -= wdt/2;
	    break;
	case Alignment::Stop:
	    xx -= wdt;
	    break;
    }
  
    int yy = y + font_->ascent() - y0;
    switch( al.ver ){
	case Alignment::Middle:
	    yy -= hgt/2;
	    break;
	case Alignment::Stop:
	    yy -= hgt;
	    break;
    }

    int overshoot = (xx+wdt) - getDevWidth();
    if( overshoot > 0 )
    {
	if( 4*(wdt - overshoot) >=  wdt )
	    xx -= overshoot;
	else return;
    }
    if( xx < 0 )
    {
	if( 4*(wdt+xx) >= wdt )
	    xx=0;
	else return;
    }

    overshoot = yy - getDevHeight();
    if( overshoot > 0)
    {
	if( (hgt - overshoot)*4 >= hgt )
	    yy -= overshoot;
	else return;
    }
    if( yy < hgt )
    {
	if( yy*4 >= hgt )
	    yy = hgt;
	else return;
    }

    if (erase) mQPainter->fillRect( xx, yy-hgt, wdt, hgt, 
				    mQPainter->backgroundColor() );
    mQPainter->drawText( xx, yy, QString(txt), len );
}

void ioDrawTool::drawEllipse ( int x, int y, int w, int h )
{
    if( !active() && !beginDraw()) return;
    mQPainter->drawEllipse ( x - x0, y - y0, w, h );
}

void ioDrawTool::drawRect ( int x, int y, int w, int h )
{
    if( !active() && !beginDraw()) return;
    mQPainter->drawRect ( x - x0, y - y0, w, h );
}


Color ioDrawTool::backgroundColor() const
{
    return Color( mQPainter->backgroundColor().rgb() );
}


void ioDrawTool::setBackgroundColor( const Color& c )
{
    if( !active() && !beginDraw()) return;
    mQPainter->setBackgroundColor( QColor( QRgb( c.rgb() )));
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


void ioDrawTool::drawPixmap (const uiPoint destTopLeft, ioPixmap* pm,
						 const uiRect srcAreaInPixmap )
{
    if ( !pm ) 
    { 
	pErrMsg( "ioDrawTool::drawPixmap : No pixmap passed" ); 
	return; 
    }

    if( !active() && !beginDraw()) return;

    QRect  src ( QPoint( srcAreaInPixmap.left(), srcAreaInPixmap.top() ),
		 QPoint( srcAreaInPixmap.right(), srcAreaInPixmap.bottom() ) );
    
    QPoint dest ( destTopLeft.x() - x0, destTopLeft.y() - y0 );

    mQPainter->drawPixmap ( dest, *pm->Pixmap() , src);

}

int ioDrawTool::getDevHeight() const
//! \return height in pixels of the device we're drawing on
{
    if( !mQPaintDevMetrics )
    { 
        if( !mQPaintDev )  
	{
	    pErrMsg("No paint device! Can not construct QPaintDevMetrics");
	    return 0;
	}
	const_cast<ioDrawTool*>(this)->mQPaintDevMetrics
			= new QPaintDeviceMetrics( mQPaintDev );
    }

    return mQPaintDevMetrics->height();
}

int ioDrawTool::getDevWidth() const
//! \return width in pixels of the device we're drawing on
{
    if( !mQPaintDevMetrics )
    {
        if( !mQPaintDev )  
	{
	    pErrMsg("No paint device! Can not construct QPaintDevMetrics");
	    return 0;
	}
	const_cast<ioDrawTool*>(this)->mQPaintDevMetrics
			= new QPaintDeviceMetrics( mQPaintDev );
    }   

    return mQPaintDevMetrics->width(); 
}


void ioDrawTool::drawMarker( uiPoint pt, const MarkerStyle& mstyle,
			    const char* txt, bool below )
{
    setPenColor( mstyle.color );
    switch ( mstyle.type )
    {
    case MarkerStyle::Square:
	drawRect( pt.x()-mstyle.size, pt.y()-mstyle.size,
		  pt.x()+mstyle.size, pt.y()+mstyle.size );
    break;
    case MarkerStyle::Circle:
	drawEllipse( uiRect(pt.x()-mstyle.size,pt.y()-mstyle.size,
			    pt.x()+mstyle.size,pt.y()+mstyle.size) );
    break;
    case MarkerStyle::Cross:
	drawLine( pt.x()-mstyle.size, pt.y()-mstyle.size,
		  pt.x()+mstyle.size, pt.y()+mstyle.size );
	drawLine( pt.x()-mstyle.size, pt.y()+mstyle.size,
		  pt.x()+mstyle.size, pt.y()-mstyle.size );
    break;
    }

    if ( txt && *txt )
    {
	Alignment al( Alignment::Middle, Alignment::Middle );
	int yoffs = 0;
	if ( mstyle.type != MarkerStyle::None )
	{
	    al.ver = below ? Alignment::Start : Alignment::Stop;
	    yoffs = below ? mstyle.size+1 : -mstyle.size-1;
	}
	drawText( uiPoint(pt.x(),pt.y()+yoffs), txt, al, false );
    }
}


void ioDrawTool::setLineStyle( LineStyle ls )
{
    mQPen.setStyle( (Qt::PenStyle) ls.type);
    mQPen.setColor( QColor( QRgb( ls.color.rgb() )));
    mQPen.setWidth( ls.width );

    if( !active() ) return;
    mQPainter->setPen( mQPen ); 
}


void ioDrawTool::setPenColor( Color colr )
{
    mQPen.setColor( QColor( QRgb( colr.rgb() )));
    if( mQPainter ) mQPainter->setPen( mQPen ); 
}


void ioDrawTool::setFillColor( Color colr )
{ 
    if( !active() && !beginDraw() ) return;

    mQPainter->setBrush( QBrush( QColor( QRgb(colr.rgb()) ) ) ) ;
}


void ioDrawTool::setPenWidth( unsigned int w )
{
    mQPen.setWidth( w );

    if( !active() ) return;
    mQPainter->setPen( mQPen ); 
}


void ioDrawTool::setFont( const uiFont& f )
{
    font_ = &f;
    if( !active() ) return;
    mQPainter->setFont( font_->qFont() );
}

/*! start drawing
\return true if active

*/
bool ioDrawTool::beginDraw()
{
    if( !mQPaintDev && !mQPainter ) return false;

    if( !mQPainter) 
    {
	mQPainter = new QPainter;
	freeMQPainter = true;
    }

    if ( !active_ ) 
    {
        if( !mQPainter->isActive() )
	{
	    if( !mQPaintDev || !mQPainter->begin( mQPaintDev ))
            {
		pErrMsg("Unable to make painter active!");
		return false;
            }
	}  

	active_ = true;
    }

    mQPainter->setPen( mQPen ); 
    mQPainter->setFont( font_->qFont() ); 

    return true;
}

bool ioDrawTool::endDraw()
{
    if( !mQPainter ) return false;
    
    if( active_ )
    {   
	mQPainter->flush();
	mQPainter->end();
        active_ = false;
    }

    if ( freeMQPainter ) delete mQPainter;
    mQPainter = 0;

    return true;
}

bool ioDrawTool::setActivePainter( QPainter* p )
{
    if( mQPainter ) return false;
    if( active_ ) return false;

    mQPainter = p;
    freeMQPainter = false;
    active_ = true;

    return true;
}
