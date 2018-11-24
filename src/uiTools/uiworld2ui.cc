/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          September 2006
 ________________________________________________________________________

-*/

#include "uiworld2ui.h"

#include "ranges.h"
#include "axislayout.h"
#include "survinfo.h"

World2UiData::World2UiData()
{}


World2UiData::World2UiData( uiSize s, const uiWorldRect& w )
    : sz(s), wr(w)
{}


World2UiData::World2UiData( const uiWorldRect& w, uiSize s )
    : sz(s), wr(w)
{}


inline static Coord crd( const Geom::Point2D<double>& p )
{ return Coord(p.x_,p.y_); }


bool World2UiData::operator ==( const World2UiData& w2ud ) const
{
    return sz == w2ud.sz
	    && crd(wr.topLeft()) == crd(w2ud.wr.topLeft())
	    && crd(wr.bottomRight()) == crd(w2ud.wr.bottomRight());
}


uiWorld2Ui::uiWorld2Ui()
{
}


uiWorld2Ui::uiWorld2Ui( const uiSize& sz, const uiWorldRect& wr )
{
    set(sz,wr);
}


uiWorld2Ui::uiWorld2Ui( const uiRect& rc, const uiWorldRect& wr )
{
    set( rc, wr );
}


uiWorld2Ui::uiWorld2Ui( const uiWorldRect& wr, const uiSize& sz )
{
    set( sz, wr );
}


uiWorld2Ui::uiWorld2Ui( const World2UiData& w )
{
    set( w.sz, w.wr );
}


bool uiWorld2Ui::operator==( const uiWorld2Ui& ) const
{
    pErrMsg("Not implemented.");
    return false;
}


void uiWorld2Ui::set( const World2UiData& w )
{ set( w.sz, w.wr ); }


void uiWorld2Ui::set( const uiWorldRect& wr, const uiSize& sz )
{ set( sz, wr ); }


void uiWorld2Ui::set( uiRect rc, const SurveyInfo& si )
{
    if ( !rc.hNrPics() || !rc.vNrPics() )
	return;

    Coord mincoord = si.minCoord();
    Coord maxcoord = si.maxCoord();
    Coord diff = maxcoord - mincoord;
    double xfac = diff.x_ / rc.hNrPics();
    double yfac = diff.y_ / rc.vNrPics();
    if ( xfac > yfac )
    {
	int extrapix = rc.vNrPics() - mNINT32( diff.y_ / xfac );
	rc.setTop( rc.top() + extrapix / 2 );
	rc.setBottom( rc.bottom() - extrapix + extrapix / 2 );
    }
    else
    {
	int extrapix = rc.hNrPics() - mNINT32( diff.x_ / yfac );
	rc.setLeft( rc.left() + extrapix / 2 );
	rc.setRight( rc.right() - extrapix + extrapix / 2 );
    }

    uiWorldRect wr( mincoord.x_, maxcoord.y_, maxcoord.x_, mincoord.y_ );
    set( rc, wr );
}


void uiWorld2Ui::set( const uiSize& sz, const uiWorldRect& wr )
{
    w2ud = World2UiData( sz, wr );
    p0 = wr.topLeft();
    fac.setXY( wr.width()/(sz.hNrPics()-1),
    wr.height()/(sz.vNrPics()-1));
    if ( wr.left() > wr.right() ) fac.x_ = -fac.x_;
    if ( wr.top() > wr.bottom() ) fac.y_ = -fac.y_;
    wrdrect_ = wr;
    uisize_ = sz;
    uiorigin.x_ = 0; uiorigin.y_ = 0;
}


void uiWorld2Ui::set( const uiRect& rc, const uiWorldRect& wr )
{
    if ( rc.hNrPics()<2 || rc.vNrPics()<2 )
	return;

    uiSize sz( rc.hNrPics(), rc.vNrPics() );
    set( sz, wr );
    uiorigin = rc.topLeft();
}


void uiWorld2Ui::setRemap( const uiSize& sz, const uiWorldRect& wrdrc )
{
    uiWorldRect wr = wrdrc;
    // Recalculate a 'good' left/right boundary
    float left, right;
    getAppropriateRange((float) wr.left(),(float) wr.right(), left, right);
    wr.setRight( right ); wr.setLeft( left );
    // recalculate a 'good' top/bottom boundary
    float top, bot;
    getAppropriateRange((float) wr.bottom(), (float) wr.top(), bot, top);
    wr.setTop( top ); wr.setBottom( bot );

    set( sz, wr );
}


void uiWorld2Ui::setRemap( const uiRect& rc, const uiWorldRect& wrdrc )
{
    const uiSize sz( rc.hNrPics(), rc.vNrPics() );
    setRemap( sz, wrdrc );
    uiorigin = rc.topLeft();
}


void uiWorld2Ui::setCartesianRemap( const uiSize& sz,
float minx, float maxx, float miny, float maxy )
{
    const uiWorldRect wr( minx, maxy, maxx, miny );
    setRemap( sz, wr );
}


void uiWorld2Ui::setCartesianRemap( const uiRect& rc,
float minx, float maxx, float miny, float maxy )
{
    const uiWorldRect wr( minx, maxy, maxx, miny );
    setRemap( rc, wr );
}


void uiWorld2Ui::resetUiRect( const uiRect& rc )
{ set( rc, wrdrect_ ); }


const World2UiData& uiWorld2Ui::world2UiData() const
{ return w2ud; }


uiWorldPoint uiWorld2Ui::transform( uiPoint p ) const
{
    return uiWorldPoint( toWorldX( p.x_ ), toWorldY( p.y_ ) );
}


uiWorldRect uiWorld2Ui::transform( uiRect area ) const
{
    return uiWorldRect( transform(area.topLeft()),
			transform(area.bottomRight()) );
}


uiPoint uiWorld2Ui::transform( uiWorldPoint p ) const
{
    return uiPoint( toUiX( (float) p.x_ ), toUiY( (float) p.y_ ) );
}


uiRect uiWorld2Ui::transform( uiWorldRect area ) const
{
    return uiRect( transform(area.topLeft()),
		   transform(area.bottomRight()) );
}

// Since the compiler will be confused if two functions
// only differ in return type, Geom::Point2D<float> is
// set rather than be returned.


void uiWorld2Ui::transform( const uiPoint& upt,
			    Geom::Point2D<float>& pt ) const
{ pt.setXY( toWorldX(upt.x_), toWorldY(upt.y_) ); }


void uiWorld2Ui::transform( const Geom::Point2D<float>& pt, uiPoint& upt) const
{  upt.setXY( toUiX( pt.x_ ), toUiY( pt.y_ ) ); }


int uiWorld2Ui::toUiX ( float wrdx ) const
{ return (int)((wrdx-p0.x_)/fac.x_+uiorigin.x_+.5); }


int uiWorld2Ui::toUiY ( float wrdy ) const
{ return (int)((wrdy-p0.y_)/fac.y_+uiorigin.y_+.5); }


float uiWorld2Ui::toWorldX ( int uix ) const
{ return (float) ( p0.x_ + (uix-uiorigin.x_)*fac.x_ ); }


float uiWorld2Ui::toWorldY ( int uiy ) const
{ return (float) ( p0.y_ + (uiy-uiorigin.y_)*fac.y_ ); }


uiWorldPoint uiWorld2Ui::origin() const
{ return p0; }


uiWorldPoint uiWorld2Ui::worldPerPixel() const
{ return fac; }


void uiWorld2Ui::getWorldXRange( float& xmin, float& xmax ) const
{
    xmax=(float) wrdrect_.right();  xmin=(float) wrdrect_.left();
    if ( xmin > xmax )
	std::swap( xmax, xmin );
}


void uiWorld2Ui::getWorldYRange( float& ymin, float& ymax ) const
{
    ymax=(float) wrdrect_.top();  ymin=(float) wrdrect_.bottom();
    if ( ymin > ymax )
	std::swap( ymax, ymin );
}


void uiWorld2Ui::getAppropriateRange( float min, float max,
				     float& newmin, float& newmax )
{
    bool rev = min > max;
    if ( rev )
	std::swap( min, max );
    Interval<float> intv( min, max );
    AxisLayout<float> al( intv );
    newmin = al.sd_.start;
    newmax = al.findEnd( max );
    if ( rev )
	std::swap( newmin, newmax );
}
