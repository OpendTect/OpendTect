#ifndef uiworld2ui_h
#define uiworld2ui_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          13/8/2000
 RCS:           $Id: uiworld2ui.h,v 1.4 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/
#include "uigeom.h"
#include "posgeom.h"

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


class uiWorld2Ui
{
public:

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
			    if ( wr.left() > wr.right() ) fac.setX( -fac.x() );
			    if ( wr.top() > wr.bottom() ) fac.setY( -fac.y() );
			}

    inline const World2UiData&	world2UiData() const
			{ return w2ud; }


    inline uiWorldPoint	transform( uiPoint p ) const
			{
			    return uiWorldPoint( p0.x() + p.x()*fac.x(),
						 p0.y() + p.y()*fac.y() );
			}
    inline uiWorldRect	transform( uiRect area ) const
			{
			    return uiWorldRect( transform(area.topLeft()),
						transform(area.bottomRight()) );
			}
    inline uiPoint	transform( uiWorldPoint p ) const
			{
			    return uiPoint( (int)((p.x()-p0.x())/fac.x()+.5),
					    (int)((p.y()-p0.y())/fac.y()+.5) );
			}
    inline uiRect	transform( uiWorldRect area ) const
			{
			    return uiRect( transform(area.topLeft()),
					   transform(area.bottomRight()) );
			}

    uiWorldPoint	origin() const		{ return p0; }
    uiWorldPoint	worldPerPixel() const	{ return fac; }
			//!< numbers may be negative

protected:

    World2UiData	w2ud;

    uiWorldPoint	p0;
    uiWorldPoint	fac;

};


#endif
