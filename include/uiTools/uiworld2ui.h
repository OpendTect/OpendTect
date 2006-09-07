#ifndef uiworld2ui_h
#define uiworld2ui_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          13/8/2000
 RCS:           $Id: uiworld2ui.h,v 1.9 2006-09-07 15:44:24 cvskris Exp $
________________________________________________________________________

-*/
#include "uigeom.h"
#include "posgeom.h"
#include "ranges.h"
#include "linear.h"

class World2UiData
{
public:

		World2UiData()		{}
		World2UiData( uiSize s, const uiWorldRect& w )
		: sz(s), wr(w)		{}
		World2UiData( const uiWorldRect& w, uiSize s )
		: sz(s), wr(w)		{}

    inline bool	operator ==( const World2UiData& w2ud ) const
		{
		    return sz == w2ud.sz
		    && crd(wr.topLeft())     == crd(w2ud.wr.topLeft())
		    && crd(wr.bottomRight()) == crd(w2ud.wr.bottomRight());
		}
    inline bool	operator !=( const World2UiData& w2ud ) const
		{ return !( *this == w2ud ); }

    uiSize	sz;
    uiWorldRect	wr;

};


/*!\brief Class to provide coordinate conversion between a cartesian coordinate
  system(or any other transformed cartesian) and UI coordinate system(screen
  coordinate system)
  
  Use the constructor or call set() to set up the two coordinate systems.
  1) If the origin of UI is not at (0,0), use uiRect instead of uiSize to set
     up the UI coordinates.
  2) In many cases the 'border' of world coordinate system is not at a good
     place, e.x. X ( 0.047 - 0.987 ). we would prefer to set it to an more
     appropriate range ( 0 - 1 ) and re-map this range to the UI window. This 
     is done by calling setRemap() or setCartesianRemap(). The proper range is
     estimated by these functions and coordinate conversion will be based on
     the new wolrd X/Y range.
 */

class uiWorld2Ui
{
public:

			uiWorld2Ui()
				{ };
			uiWorld2Ui( uiSize sz, const uiWorldRect& wr )
						{ set(sz,wr); }
			uiWorld2Ui( const uiWorldRect& wr, uiSize sz )
						{ set(sz,wr); }
			uiWorld2Ui( const World2UiData& w )
						{ set( w.sz, w.wr ); }

    inline void		set( const World2UiData& w )
			{ set( w.sz, w.wr ); }
    inline void		set( const uiWorldRect& wr, uiSize sz )
			{ set( sz, wr ); }
    void		set( uiSize sz, const uiWorldRect& wr )
			{
			    w2ud = World2UiData( sz, wr );
			    p0 = wr.topLeft();
			    fac.setXY( wr.width()/(sz.hNrPics()-1),
				       wr.height()/(sz.vNrPics()-1));
			    if ( wr.left() > wr.right() ) fac.x = -fac.x;
			    if ( wr.top() > wr.bottom() ) fac.y = -fac.y;
			    wrdrect_ = wr;
			    uisize_ = sz;
			    uiorigin.x = 0; uiorigin.y = 0;
			}
    void		set( const uiRect& rc, const uiWorldRect& wr )
			{
			    uiSize sz( rc.hNrPics(), rc.vNrPics(), true );
			    set( sz, wr );
			    uiorigin = rc.topLeft();
			}
    void		setRemap( const uiSize& sz, const uiWorldRect& wrdrc )
			{
			    uiWorldRect wr = wrdrc;
			    // Recalculate a 'good' left/right boundary
			    float left, right;
			    getAppopriateRange(wr.left(), wr.right(),
			    		       left, right);
			    wr.setRight( right ); wr.setLeft( left );
			    // recalculate a 'good' top/bottom boundary
			    float top, bot;
			    getAppopriateRange(wr.bottom(), wr.top(), bot, top);
			    wr.setTop( top ); wr.setBottom( bot );
			    
			    set( sz, wr );
			}
    void		setRemap( const uiRect& rc, const uiWorldRect& wrdrc )
			{
			    uiSize sz( rc.hNrPics(), rc.vNrPics(), true );
			    setRemap( sz, wrdrc );
			    uiorigin = rc.topLeft();
			}
			//! For most cases we are dealing with cartesian coord.
			//! system. Just pass the X/Y range to set up
    void		setCartesianRemap( const uiSize& sz,
    				float minx, float maxx, float miny, float maxy )
			{
			    uiWorldRect wr = xyRng2WrdRect(minx,maxx,miny,maxy);
			    setRemap( sz, wr );
			}
    void		setCartesianRemap( const uiRect& rc,
    				float minx, float maxx, float miny, float maxy )
			{
			    uiWorldRect wr = xyRng2WrdRect(minx,maxx,miny,maxy);
			    setRemap( rc, wr );
			}
			
			//! The following reset...() functions must be called
			//! ONLY AFTER a call to set...() is made.
    void		resetUiSize( const uiSize& sz )
    				{ set( sz, wrdrect_ ); }
    void		resetUiRect( const uiRect& rc )
    				{ set( rc, wrdrect_ ); }
    void		resetWorldRect( const uiWorldRect& wr )
    				{ set( uisize_, wr ); }
    void		resetWorldRectRemap( const uiWorldRect& wr )
    				{ setRemap( uisize_, wr ); }

    inline const World2UiData&	world2UiData() const
			{ return w2ud; }


    inline uiWorldPoint	transform( uiPoint p ) const
			{
			    return uiWorldPoint( toWorldX( p.x ),
			    			 toWorldY( p.y ) );
			}
    inline uiWorldRect	transform( uiRect area ) const
			{
			    return uiWorldRect( transform(area.topLeft()),
						transform(area.bottomRight()) );
			}
    inline uiPoint	transform( uiWorldPoint p ) const
			{
			    return uiPoint( toUiX( p.x ), toUiY( p.y ) );
			}
    inline uiRect	transform( uiWorldRect area ) const
			{
			    return uiRect( transform(area.topLeft()),
					   transform(area.bottomRight()) );
			}
			// Since the compiler will be comfused if two functions
			// only differ in return type, Geom::Point2D<float> is
			// set rather than be returned.
    inline void		transform( const uiPoint& upt,
	    			   Geom::Point2D<float>& pt ) const
    			{ pt.setXY( toWorldX(upt.x), toWorldY(upt.y) ); }
    inline void		transform( const Geom::Point2D<float>& pt,
	    			   uiPoint& upt) const
    			{  upt.setXY( toUiX( pt.x ), toUiY( pt.y ) ); };
    
    inline int		toUiX ( float wrdx ) const
    			{ return (int)((wrdx-p0.x)/fac.x+uiorigin.x+.5); }
    inline int		toUiY ( float wrdy ) const
    			{ return (int)((wrdy-p0.y)/fac.y+uiorigin.y+.5); }
    inline float	toWorldX ( int uix ) const
    			{ return p0.x + (uix-uiorigin.x)*fac.x; }
    inline float	toWorldY ( int uiy ) const
    			{ return p0.y + (uiy-uiorigin.y)*fac.y; }

    uiWorldRect		xyRng2WrdRect( float minx, float maxx,
    				       float miny, float maxy )
			    { return uiWorldRect( minx, maxy, maxx, miny ); }
			

    uiWorldPoint	origin() const		{ return p0; }
    uiWorldPoint	worldPerPixel() const	{ return fac; }
			//!< numbers may be negative
			
			//! If the world X/Y range has been re-calculated by 
			//! calling setRemap() or setCartesianRemap(), call
			//! these two functions to get the new value
    void		getWorldXRange( float& xmin, float& xmax ) const
			{
			    xmax=wrdrect_.right();  xmin=wrdrect_.left();
			    if ( xmin > xmax ) Swap( xmax, xmin ); 
			}
    void		getWorldYRange( float& ymin, float& ymax ) const
			{
			    ymax=wrdrect_.top();  ymin=wrdrect_.bottom();
			    if ( ymin > ymax ) Swap( ymax, ymin ); 
			}
			//! Get a recommended step value for annotating X/Y axis
			//! The step value will always be positive which means
			//! stepping from min. to max. value
    void		getRecmMarkStep( float& xstep, float& ystep ) const
    			{
			    float min, max;
			    getWorldXRange( min, max );
			    getRecmMarkStep( min, max, xstep );
			    getWorldYRange( min, max );
			    getRecmMarkStep( min, max, ystep );
			}

protected:

    World2UiData	w2ud;

    uiWorldPoint	p0;
    uiWorldPoint	fac;

    uiWorldRect		wrdrect_; //!< the world rect used for coord. conv.
    uiSize		uisize_;
    uiPoint		uiorigin;

private:
			void getAppopriateRange( float min, float max,
						 float& newmin, float& newmax )
			{
			    bool rev = min > max;
			    if ( rev )	Swap( min, max );
			    Interval<float> intv( min, max );
			    AxisLayout al( intv );
			    newmin = al.sd.start;
			    newmax = al.findEnd( max );
			    if ( rev )	Swap( newmin, newmax );
			}
			void getRecmMarkStep( float min, float max,
					      float& step ) const
			{
			    if( min > max ) Swap( min, max );
			    Interval<float> intv( min, max );
			    AxisLayout al( intv );
			    step = al.sd.step;
			}

};


#endif
