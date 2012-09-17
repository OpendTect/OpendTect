/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/12/1999
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: iodraw.cc,v 1.55 2012/09/13 11:27:27 cvsbert Exp $";

#include "iodrawtool.h"
#include "iodrawimpl.h"

#include "color.h"
#include "draw.h"
#include "errh.h"
#include "bufstringset.h"
#include "pixmap.h"
#include "uifont.h"

#include <QBrush>
#include <QPainter>
#include <QPen>
#include <QPolygon>

#include <math.h>

static const int cNoFillType = 0;
static const int cDotsFillType = 1;
static const int cLinesFillType = 2;


void ioDrawTool::getFillPatternTypes( BufferStringSet& res )
{
    res.add( "No Fill" );
    res.add( "Dots" );
    res.add( "Lines" );
}


void ioDrawTool::getFillPatternOpts( int fp, BufferStringSet& res )
{
    res.setEmpty();
    if ( fp == cDotsFillType )
    {
	res.add( "Uniform color" );
	res.add( "Extremely dense" );
	res.add( "Very dense" );
	res.add( "Somewhat dense" );
	res.add( "Half dense" );
	res.add( "Somewhat sparse" );
	res.add( "Very sparse" );
	res.add( "Extremely sparse" );
    }
    else if ( fp == cLinesFillType )
    {
	res.add( "Horizontal lines" );
	res.add( "Vertical lines" );
	res.add( "Crossing horizontal and vertical lines" );
	res.add( "Backward diagonal lines" );
	res.add( "Forward diagonal lines" );
	res.add( "Crossing diagonal lines" );
    }
    // else none
}


ioDrawTool::ioDrawTool( QPaintDevice* pd )
    : qpainter_(0)
    , qpaintermine_(false)
    , qpainterprepared_(false)
    , qpen_(*new QPen())
    , qpaintdev_(*pd)
    , font_(&FontList().get())
    , areabgcolor_(Color::White())
    , usebgpattern_(false)
{
    if ( !pd )
	pErrMsg( "Null paint device passed. Crash will follow" );

    setLineStyle( LineStyle() );
}


ioDrawTool::~ioDrawTool() 
{ 
    if ( qpaintermine_ )
	delete qpainter_;

    delete &qpen_;
}


void ioDrawTool::translate( float dx, float dy )
{
    preparePainter();
    qpainter_->translate( dx, dy );
}


void ioDrawTool::rotate( float angle )
{
    preparePainter();
    qpainter_->rotate( angle );
}


void ioDrawTool::drawText( int x, int y, const char* txt, const Alignment& al )
{
    preparePainter();

    uiPoint bl( x, y );
    const uiSize sz( font_->width(txt), font_->ascent() );
    if ( al.hPos() == Alignment::HCenter )
	bl.x -= sz.width() / 2;
    else if ( al.hPos() == Alignment::Right )
	bl.x -= sz.width();
    if ( al.vPos() == Alignment::VCenter )
	bl.y += sz.height() / 2;
    else if ( al.vPos() == Alignment::Bottom )
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


inline
static uiPoint getEndPoint( const uiPoint& pt, double angle, double len )
{
    uiPoint endpt( pt );
    double delta = len * cos( angle );
    endpt.x += mNINT32(delta);
    // In UI, Y is positive downward
    delta = -len * sin( angle );
    endpt.y += mNINT32(delta);
    return endpt;
}


void ioDrawTool::drawLine( const uiPoint& pt, double angle, double len )
{
    drawLine( pt, getEndPoint(pt,angle,len) );
}


void ioDrawTool::drawLine( const TypeSet<uiPoint>& pts, bool close )
{
    preparePainter();
    const int nrpoints = pts.size();
    if ( nrpoints < 2 ) return;

    QPolygon qarray( nrpoints );
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


void ioDrawTool::drawEllipse ( int x, int y, int rx, int ry )
{
    preparePainter();
    qpainter_->drawEllipse( QRect( x-rx, y-ry, rx*2, ry*2 ) );
}


void ioDrawTool::drawEllipse( const uiPoint& center, const uiSize& sz )
{
    drawEllipse( center.x, center.y, sz.hNrPics(), sz.vNrPics() );
}


void ioDrawTool::drawHalfSquare( const uiPoint& from, const uiPoint& to,
				 bool left )
{
    // Alas, qpainter_->drawArc can only draw arcs that are horizontal ...
    // In UI, Y is positive downward

    // The bounding rectangle's coords are:
    // 
    const uiPoint halfrel( (to.x-from.x)/2, (to.y-from.y)/2 );
    if ( left )
    {
	drawLine( from.x, from.y, from.x + halfrel.y, from.y - halfrel.x );
	drawLine( from.x + halfrel.y, from.y - halfrel.x,
		  to.x + halfrel.y, to.y - halfrel.x );
	drawLine( to.x + halfrel.y, to.y - halfrel.x, to.x, to.y );
    }
    else
    {
	drawLine( from.x, from.y, from.x - halfrel.y, from.y + halfrel.x );
	drawLine( from.x - halfrel.y, from.y + halfrel.x,
		  to.x - halfrel.y, to.y + halfrel.x );
	drawLine( to.x - halfrel.y, to.y + halfrel.x, to.x, to.y );
    }

}


void ioDrawTool::drawHalfSquare( const uiPoint& from, double ang, double d,
				 bool left )
{
    uiPoint to( from );
    double delta = d * cos( ang );
    to.x += mNINT32( delta );
    // In UI, Y is positive downward
    delta = -d * sin( ang );
    to.y += mNINT32( delta );
    drawHalfSquare( from, to, left );
}


void ioDrawTool::setDrawAreaBackgroundColor( const Color& col )
{
    areabgcolor_ = col;
}


void ioDrawTool::setBackgroundMode( BackgroundMode mode )
{
    preparePainter();
    Qt::BGMode qmode = (Qt::BGMode)(int)mode;
    qpainter_->setBackgroundMode( qmode );
}


ioDrawTool::BackgroundMode ioDrawTool::backgroundMode() const
{
    preparePainter();
    return (BackgroundMode)(int)qpainter_->backgroundMode();
}


Color ioDrawTool::backgroundColor() const
{
    preparePainter();
    return Color( qpainter_->background().color().rgb() );
}


void ioDrawTool::setBackgroundColor( const Color& c )
{
    preparePainter();
    QBrush brush( QColor(c.r(),c.g(),c.b()) );
    qpainter_->setBackground( brush );
}


void ioDrawTool::clear( const uiRect* r, const Color* c )
{
    preparePainter();
    const Color col = c ? *c : areabgcolor_;
    QRectF qrect = r ? QRectF( r->left(), r->top(), r->right(), r->bottom() )
		     : QRectF( 0, 0, getDevWidth(), getDevHeight() );
    qpainter_->fillRect( qrect, QColor(col.r(),col.g(),col.b()) );
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
{ return qpaintdev_.height(); }

int ioDrawTool::getDevWidth() const
{ return qpaintdev_.width(); }


void ioDrawTool::drawPoint( const uiPoint& pt, bool hl )
{
    QPoint pts[13]; int ptnr = 0;
#define mSetPt(ox,oy) pts[ptnr].setX(pt.x+ox); pts[ptnr++].setY(pt.y+oy)
    mSetPt( 0, 0 );
    mSetPt( -1, 0 ); mSetPt( 1, 0 );
    mSetPt( 0, -1 ); mSetPt( 0, 1 );
    if ( hl )
    {
	mSetPt( -1, -1 ); mSetPt( 1, -1 );
	mSetPt( -1, 1 ); mSetPt( 1, 1 );
	mSetPt( 2, 0 ); mSetPt( -2, 0 );
	mSetPt( 0, 2 ); mSetPt( 0, -2 );
    }
    qpainter_->drawPoints( pts, ptnr );
}


void ioDrawTool::drawMarker( const uiPoint& pt, const MarkerStyle2D& mstyle,
       			     float angle, int side )
{
    setPenColor( mstyle.color_ ); setFillColor( mstyle.color_ );
    if ( side != 0 )
	pErrMsg( "TODO: implement single-sided markers" );
    if ( !mIsZero(angle,1e-3) )
	pErrMsg( "TODO: implement tilted markers" );

    switch ( mstyle.type_ )
    {
    case MarkerStyle2D::Square:
	drawRect( pt.x-mstyle.size_, pt.y-mstyle.size_,
		  2*mstyle.size_, 2*mstyle.size_ );
    break;
    case MarkerStyle2D::Circle:
	drawCircle( pt.x, pt.y, mstyle.size_/2 );
    break;
    case MarkerStyle2D::Cross:
	drawLine( pt.x-mstyle.size_, pt.y-mstyle.size_,
		  pt.x+mstyle.size_, pt.y+mstyle.size_ );
	drawLine( pt.x-mstyle.size_, pt.y+mstyle.size_,
		  pt.x+mstyle.size_, pt.y-mstyle.size_ );
    break;
    }
}


static double getAddedAngle( double ang, float ratiopi )
{
    ang += ratiopi * M_PI;
    while ( ang < -M_PI ) ang += 2 * M_PI;
    while ( ang > M_PI ) ang -= 2 * M_PI;
    return ang;
}


static void drawArrowHead( ioDrawTool& dt, const ArrowHeadStyle& hs,
			   const Geom::Point2D<int>& pos,
			   const Geom::Point2D<int>& comingfrom )
{
    static const float headangfac = .82; // bigger => lines closer to main line

    // In UI, Y is positive downward
    const uiPoint relvec( pos.x - comingfrom.x, comingfrom.y - pos.y );
    const double ang( atan2((double)relvec.y,(double)relvec.x) );

    if ( hs.handedness_ == ArrowHeadStyle::TwoHanded )
    {
	switch ( hs.type_ )
	{
	case ArrowHeadStyle::Square:
	    dt.drawHalfSquare( pos, ang+M_PI, hs.sz_, true );
	    dt.drawHalfSquare( pos, ang+M_PI, hs.sz_, false );
	break;
	case ArrowHeadStyle::Cross:
	    dt.drawLine( pos, getAddedAngle(ang,.25), hs.sz_/2 );
	    dt.drawLine( pos, getAddedAngle(ang,.75), hs.sz_/2 );
	    dt.drawLine( pos, getAddedAngle(ang,-.25), hs.sz_/2 );
	    dt.drawLine( pos, getAddedAngle(ang,-.75), hs.sz_/2 );
	break;
	case ArrowHeadStyle::Triangle:
	case ArrowHeadStyle::Line:
	{
	    const uiPoint rightend = getEndPoint( pos,
				     getAddedAngle(ang,headangfac), hs.sz_ );
	    const uiPoint leftend = getEndPoint( pos,
				    getAddedAngle(ang,-headangfac), hs.sz_ );
	    dt.drawLine( pos, rightend ); dt.drawLine( pos, leftend );
	    if ( hs.type_ == ArrowHeadStyle::Triangle )
		dt.drawLine( leftend, rightend );
	}
	break;
	}
    }
    else
    {
	const bool isleft = hs.handedness_ == ArrowHeadStyle::LeftHanded;

	switch ( hs.type_ )
	{
	case ArrowHeadStyle::Square:
	    dt.drawHalfSquare( pos, ang+M_PI, hs.sz_, isleft );
	break;
	case ArrowHeadStyle::Cross:
	    if ( isleft )
	    {
		dt.drawLine( pos, getAddedAngle(ang,.25), hs.sz_/2 );
		dt.drawLine( pos, getAddedAngle(ang,.75), hs.sz_/2 );
	    }
	    else
	    {
		dt.drawLine( pos, getAddedAngle(ang,-.25), hs.sz_/2 );
		dt.drawLine( pos, getAddedAngle(ang,-.75), hs.sz_/2 );
	    }
	break;
	case ArrowHeadStyle::Triangle:
	case ArrowHeadStyle::Line:
	{
	    const uiPoint endpt = getEndPoint( pos,
				  getAddedAngle(ang,headangfac*(isleft?1:-1)),
				  hs.sz_ );
	    dt.drawLine( pos, endpt );
	    if ( hs.type_ == ArrowHeadStyle::Triangle  )
	    {
		const float dx = comingfrom.x - pos.x;
		const float dy = comingfrom.y - pos.y;
		const float t = ((endpt.x-pos.x)*dx + (endpt.y-pos.y)*dy)
			      / (dx*dx + dy*dy);
		const Geom::Point2D<float> fpp( (float)pos.x + t*dx,
						(float)pos.y + t*dy );
		const uiPoint pp( mNINT32(fpp.x), mNINT32(fpp.y) ); // projected pt
		dt.drawLine( endpt, pp );
	    }
	}
	break;
	}
    }

}


void ioDrawTool::drawArrow( const uiPoint& tail, const uiPoint& head,
			    const ArrowStyle& as )
{
    const LineStyle ls( lineStyle() );
    setLineStyle( as.linestyle_ );

    drawLine( tail, head );
    if ( as.hasHead() )
	drawArrowHead( *this, as.headstyle_, head, tail );
    if ( as.hasTail() )
	drawArrowHead( *this, as.tailstyle_, tail, head );

    setLineStyle( ls );
}


LineStyle ioDrawTool::lineStyle() const
{
    QColor qcol( qpen_.color() );
    Color col( qcol.red(), qcol.green(), qcol.blue() );
    return LineStyle( (LineStyle::Type)qpen_.style(), qpen_.width(), col );
}


void ioDrawTool::setLineStyle( const LineStyle& ls )
{
    qpen_.setStyle( (Qt::PenStyle)ls.type_ );
    qpen_.setColor( QColor( QRgb( ls.color_.rgb() )));
    qpen_.setWidth( ls.width_ );

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
    QBrush qbrush = qpainter_->brush();
    const QRgb qrgb( colr.rgb() );
    qbrush.setColor( qrgb );
    qpainter_->setBrush( qbrush );
}


void ioDrawTool::setFillPattern( int typ, int opt )
{
    preparePainter();
    QBrush qbrush = qpainter_->brush();

    Qt::BrushStyle qbs = Qt::NoBrush;
    if ( typ == cDotsFillType )
    {
	if ( opt < 0 || opt > 7 ) opt = 0;
	qbs = (Qt::BrushStyle)(((int)Qt::SolidPattern)+opt);
    }
    else if ( typ == cLinesFillType )
    {
	if ( opt < 0 || opt > 8 ) opt = 0;
	qbs = (Qt::BrushStyle)(((int)Qt::HorPattern)+opt);
    }

    qbrush.setStyle( qbs );
    qpainter_->setBrush( qbrush );
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
	if ( usebgpattern_ )
	{
	    qpainter_->setBackgroundMode( Qt::OpaqueMode );
	    QBrush brush;
	    brush.setColor( QColor(0,0,0) );
	    brush.setStyle( Qt::DiagCrossPattern );
	    self.qpainter_->fillRect(
		   QRectF(1,1,self.getDevWidth()-2,self.getDevHeight()-2),
		   brush );
	}
	else
	{
	    qpainter_->setBackgroundMode( Qt::TransparentMode );
	    self.qpainter_->fillRect(
		    QRectF(0,0,self.getDevWidth(),self.getDevHeight()),
		    QColor(areabgcolor_.r(),areabgcolor_.g(),areabgcolor_.b()));
	}
	self.qpainterprepared_ = true;
    }
}


void ioDrawTool::dismissPainter()
{
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
    qpainter_->setCompositionMode( QPainter::CompositionMode_Xor );
}


void ioDrawTool::setRasterNorm()
{
    preparePainter();
    qpainter_->setCompositionMode(
			    QPainter::CompositionMode_SourceOver );
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
}
