/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: iodraw.cc,v 1.26 2007-06-12 09:27:26 cvsnanne Exp $
________________________________________________________________________

-*/

#include "iodrawtool.h"
#include "iodrawimpl.h"
#include "errh.h"
#include "pixmap.h"
#include "color.h"
#include "uifont.h"
#include "draw.h"

#ifdef USEQT3
# include <qpen.h>
# include <qbrush.h>
# include <qpaintdevicemetrics.h>
# include <qpointarray.h>
# include <qpainter.h>
#else
# include <QPolygon>
# include <QPainter>
# include <QPen>
# include <QBrush>
#endif


ioDrawTool::ioDrawTool( QPaintDevice* pd )
    : qpainter_(0)
    , qpaintermine_(false)
    , qpainterprepared_(false)
    , qpen_(*new QPen())
    , qpaintdev_(*pd)
    , font_(&uiFontList::get())
#ifdef USEQT3
    , qpaintdevmetr_( 0 )
#endif
{
    if ( !pd )
	pErrMsg("Null paint device passed. Crash will follow");

    setLineStyle( LineStyle() );
}


ioDrawTool::~ioDrawTool() 
{ 
    if ( qpaintermine_ )
	delete qpainter_;

#ifdef USEQT3
    if ( qpaintdevmetr_ )
	delete qpaintdevmetr_;
#endif
    
    delete &qpen_;
}


void ioDrawTool::drawText( int x, int y, const char* txt, const Alignment& al )
{
    preparePainter();

    uiPoint bl( x, y );
    const uiSize sz( font_->width(txt), font_->ascent() );
    if ( al.hor == Alignment::Middle )
	bl.x -= sz.width() / 2;
    else if ( al.hor == Alignment::Stop )
	bl.x -= sz.width();
    if ( al.ver == Alignment::Middle )
	bl.y += sz.height() / 2;
    else if ( al.ver == Alignment::Start )
	bl.y += sz.height();

    qpainter_->drawText( bl.x, bl.y, QString(txt) );
}


void ioDrawTool::drawText( const uiPoint& p, const char* t, const Alignment& a )
{
    drawText( p.x, p.y, t, a );
}


void ioDrawTool::drawLine( int x1, int y1, int x2, int y2 )
{
    preparePainter();
    qpainter_->drawLine( x1, y1, x2, y2 );
}


void ioDrawTool::drawLine( const uiPoint& p1, const uiPoint& p2 )
{
    drawLine( p1.x, p1.y, p2.x, p2.y );
}


void ioDrawTool::drawLine( const TypeSet<uiPoint>& pts, bool close )
{
    const int nrpoints = pts.size();
    if ( nrpoints < 2 ) return;

#ifdef USEQT3
    QPointArray qarray( nrpoints );
#else
    QPolygon qarray( nrpoints );
#endif

    for ( int idx=0; idx<nrpoints; idx++ )
	qarray.setPoint( (unsigned int)idx, pts[idx].x, pts[idx].y );

    if ( close )
	qpainter_->drawPolygon( qarray );
    else
	qpainter_->drawPolyline( qarray );
}


void ioDrawTool::drawRect ( int x, int y, int w, int h )
{
    preparePainter();
    int pensize = qpainter_->pen().width();
    if ( pensize < 1 )
	{ pErrMsg("Pen size < 1"); pensize = 1; }
    qpainter_->drawRect ( x, y, w - pensize, h - pensize );
}


void ioDrawTool::drawRect( const uiPoint& topLeft, const uiSize& sz )
{
    drawRect( topLeft.x, topLeft.y, sz.hNrPics(), sz.vNrPics() );
}


void ioDrawTool::drawRect( const uiRect& r )
{
    drawRect( r.left(), r.top(), r.hNrPics(), r.vNrPics() );
}


void ioDrawTool::drawEllipse ( int x, int y, int w, int h )
{
    preparePainter();
    qpainter_->drawEllipse ( x, y, w, h );
}


void ioDrawTool::drawEllipse( const uiPoint& topLeft, const uiSize& sz )
{
    drawEllipse( topLeft.x, topLeft.y, sz.hNrPics(), sz.vNrPics());
}


void ioDrawTool::drawEllipse( const uiRect& r )
{
    drawEllipse( r.left(), r.top(), r.hNrPics(), r.vNrPics() );
}



Color ioDrawTool::backgroundColor() const
{
    preparePainter();
#ifdef USEQT3
    return Color( qpainter_->backgroundColor().rgb() );
#else
    return Color( qpainter_->background().color().rgb() );
#endif
}


void ioDrawTool::setBackgroundColor( const Color& c )
{
    preparePainter();
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
    preparePainter();
#ifndef USEQT3
    qpainter_->eraseRect( 0, 0, getDevWidth(), getDevHeight() );
    return;
#endif

    uiRect r( 0, 0, getDevWidth(), getDevHeight() );
    if ( rect ) r = *rect;
    Color c( col ? *col : backgroundColor() );
    setPenColor( c ); setFillColor( c );
    drawRect( r );
}


void ioDrawTool::drawBackgroundPixmap( const Color* col )
{
    preparePainter();
    if ( col ) setBackgroundColor( *col );
    qpainter_->setBackgroundMode( Qt::OpaqueMode );
    qpainter_->setBrush( Qt::DiagCrossPattern );
    drawRect( uiRect( 0, 0, getDevWidth(), getDevHeight() ) );
}


void ioDrawTool::drawPixmap (const uiPoint& desttl, const ioPixmap* pm,
			     const uiRect& pmsrcarea )
{
    if ( !pm || !pm->qpixmap() ) 
	{ pErrMsg( "No pixmap" ); return; }

    preparePainter();

    QRect src( QPoint(pmsrcarea.left(),pmsrcarea.top()),
	       QPoint(pmsrcarea.right(),pmsrcarea.bottom()) );
    QPoint dest( desttl.x, desttl.y );
    qpainter_->drawPixmap( dest, *pm->qpixmap(), src );
}


void ioDrawTool::drawPixmap( int left, int top, ioPixmap* pm,
			     int sleft, int stop, int sright, int sbottom )
{
    drawPixmap( uiPoint(left,top), pm, uiRect(sleft,stop,sright,sbottom));
}


int ioDrawTool::getDevHeight() const
{
#ifdef USEQT3
    if ( !qpaintdevmetr_ )
	const_cast<ioDrawTool*>(this)->qpaintdevmetr_
			    = new QPaintDeviceMetrics( &qpaintdev_ );
    return qpaintdevmetr_->height();
#else
    return qpaintdev_.height();
#endif
}


int ioDrawTool::getDevWidth() const
{
#ifdef USEQT3
    if ( !qpaintdevmetr_ )
	const_cast<ioDrawTool*>(this)->qpaintdevmetr_
			    = new QPaintDeviceMetrics( &qpaintdev_ );
    return qpaintdevmetr_->width();
#else
    return qpaintdev_.width(); 
#endif
}


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
	drawCircle( pt.x - mstyle.size, pt.y - mstyle.size, mstyle.size );
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
	drawText( uiPoint(pt.x,pt.y+yoffs), txt, al );
    }
}


void ioDrawTool::setLineStyle( const LineStyle& ls )
{
    qpen_.setStyle( (Qt::PenStyle) ls.type);
    qpen_.setColor( QColor( QRgb( ls.color.rgb() )));
    qpen_.setWidth( ls.width );

    if ( qpainter_ )
	qpainter_->setPen( qpen_ ); 
}


void ioDrawTool::setPenColor( const Color& colr )
{
    qpen_.setColor( QColor( QRgb(colr.rgb()) ) );
    if ( qpainter_ )
	qpainter_->setPen( qpen_ ); 
}


void ioDrawTool::setFillColor( const Color& colr )
{ 
    preparePainter();
    qpainter_->setBrush( QColor( QRgb(colr.rgb()) ) );
}


void ioDrawTool::setPenWidth( unsigned int w )
{
    qpen_.setWidth( w );
    if ( qpainter_ )
	qpainter_->setPen( qpen_ ); 
}


void ioDrawTool::setFont( const uiFont& f )
{
    font_ = &f;
    if ( qpainter_ )
	qpainter_->setFont( font_->qFont() );
}


void ioDrawTool::preparePainter() const
{
    if ( !qpainter_ ) 
    {
	ioDrawTool& self = *const_cast<ioDrawTool*>( this );
	self.qpainter_ = new QPainter( &qpaintdev_ );
	self.qpaintermine_ = true;
	self.qpainterprepared_ = false;
    }

    if ( !qpainterprepared_ )
    {
	ioDrawTool& self = *const_cast<ioDrawTool*>( this );
	self.qpainter_->setPen( qpen_ ); 
	self.qpainter_->setFont( font_->qFont() ); 
	self.qpainterprepared_ = true;
    }
}


void ioDrawTool::dismissPainter()
{
#ifdef USEQT3
    if ( qpainter_ )
	qpainter_->flush();
#endif

    if ( qpaintermine_ )
	delete qpainter_;
    qpainter_ = 0;
    qpaintermine_ = true;
    qpainterprepared_ = false;
}


void ioDrawTool::setActivePainter( QPainter* p )
{
    if ( p == qpainter_ ) return;

    dismissPainter();
    qpainter_ = p;
    qpaintermine_ = false;
}


void ioDrawTool::setRasterXor()
{
    preparePainter();
#ifdef USEQT3
    qpainter_->setRasterOp( Qt::XorROP );
#else 
    qpainter_->setCompositionMode( QPainter::CompositionMode_Xor );
#endif
}


void ioDrawTool::setRasterNorm()
{
    preparePainter();
#ifdef USEQT3
    qpainter_->setRasterOp( Qt::CopyROP );
#else
    qpainter_->setCompositionMode(
			    QPainter::CompositionMode_SourceOver );
#endif
}


void ioDrawAreaImpl::releaseDrawTool()
{
    delete dt_; dt_ = 0;
}


ioDrawTool& ioDrawAreaImpl::drawTool()
{
    if ( !dt_ )
	dt_ = new ioDrawTool( qPaintDevice() );
    return *dt_;
};
