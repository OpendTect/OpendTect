#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigeom.h"

class SurveyInfo;

/*!
\brief
*/

mExpClass(uiTools) World2UiData
{
public:
		World2UiData();
		World2UiData( uiSize s, const uiWorldRect& w );
		World2UiData( const uiWorldRect& w, uiSize s );
    virtual	~World2UiData();

    bool	operator ==( const World2UiData& w2ud ) const;
    bool	operator !=( const World2UiData& w2ud ) const;

    uiSize	sz;
    uiWorldRect	wr;
};


/*!
\brief Class to provide coordinate conversion between a cartesian coordinate
system (or any other transformed cartesian) and UI coordinate system(screen
coordinate system.)

  Use the constructor or call set() to set up the two coordinate systems.
  1) If the origin of UI is not at (0,0), use uiRect instead of uiSize to set
     up the UI coordinates.
  2) In many cases the 'border' of world coordinate system is not at a good
     place, e.x. X ( 0.047 - 0.987 ). we would prefer to set it to an more
     appropriate range ( 0 - 1 ) and re-map this range to the UI window. This
     is done by calling setRemap() or setCartesianRemap(). The proper range is
     estimated by these functions and coordinate conversion will be based on
     the new world X/Y range.
*/

mExpClass(uiTools) uiWorld2Ui
{
public:
			uiWorld2Ui();
			uiWorld2Ui( const uiSize& sz, const uiWorldRect& wr );
			uiWorld2Ui( const uiRect& rc, const uiWorldRect& wr );
			uiWorld2Ui( const uiWorldRect& wr, const uiSize& sz );
			uiWorld2Ui( const World2UiData& w );
    virtual		~uiWorld2Ui();

    bool		operator==(const uiWorld2Ui& ) const;

    void		set( const World2UiData& w );
    void		set( const uiWorldRect& wr, const uiSize& sz );
    void		set( const uiSize& sz, const uiWorldRect& wr );
    void		set( const uiRect& rc, const uiWorldRect& wr );
    void		set( uiRect rc, const SurveyInfo& si );
			//! Quite useful for survey level maps.

    void		setRemap( const uiSize& sz, const uiWorldRect& wrdrc );
    void		setRemap( const uiRect& rc, const uiWorldRect& wrdrc );
			//! For most cases we are dealing with cartesian coord.
			//! system. Just pass the X/Y range to set up
    void		setCartesianRemap( const uiSize& sz,
			    float minx, float maxx, float miny, float maxy );
    void		setCartesianRemap( const uiRect& rc,
			    float minx, float maxx, float miny, float maxy );

			//! The following reset...() functions must be called
			//! ONLY AFTER a call to set...() is made.
    void		resetUiRect( const uiRect& rc );

    const World2UiData&	world2UiData() const;


    uiWorldPoint	transform( uiPoint p ) const;
    uiWorldRect		transform( uiRect area ) const;
    uiPoint		transform( uiWorldPoint p ) const;
    uiRect		transform( uiWorldRect area ) const;
			// Since the compiler will be confused if two functions
			// only differ in return type, Geom::Point2D<float> is
			// set rather than be returned.
    void		transform(const uiPoint& upt,
				  Geom::Point2D<float>& pt) const;
    void		transform(const Geom::Point2D<float>& pt,
				  uiPoint& upt) const;

    int			toUiX(float wrdx) const;
    int			toUiY(float wrdy) const;
    float		toWorldX(int uix) const;
    float		toWorldY(int uiy) const;

    template <class TT,class FT>
    TT			transformX(FT x,bool toworld) const;
    template <class TT,class FT>
    TT			transformY(FT y,bool toworld) const;
    template <class TT,class FT>
    Geom::Point2D<TT>	transform(const Geom::Point2D<FT>&,bool toworld) const;

    uiWorldPoint	origin() const;
    uiWorldPoint	worldPerPixel() const;
			//!< numbers may be negative

			//! If the world X/Y range has been re-calculated by
			//! calling setRemap() or setCartesianRemap(), call
			//! these two functions to get the new value
    void		getWorldXRange( float& xmin, float& xmax ) const;
    void		getWorldYRange( float& ymin, float& ymax ) const;

protected:

    World2UiData	w2ud;

    uiWorldPoint	p0;
    uiWorldPoint	fac;

    uiWorldRect		wrdrect_; //!< the world rect used for coord. conv.
    uiSize		uisize_;
    uiPoint		uiorigin;

private:
			void getAppropriateRange( float min, float max,
						 float& newmin, float& newmax );
};


template <class TT,class FT>
TT uiWorld2Ui::transformX( FT x, bool toworld ) const
{
    if ( toworld )
	return p0.x + (x-uiorigin.x)*fac.x;
    else
	return (x-p0.x)/fac.x + uiorigin.x;
}

template <class TT,class FT>
TT uiWorld2Ui::transformY( FT y, bool toworld ) const
{
    if ( toworld )
	return p0.y + (y-uiorigin.y)*fac.y;
    else
	return (y-p0.y)/fac.y + uiorigin.y;
}


template <class TT,class FT>
Geom::Point2D<TT> uiWorld2Ui::transform( const Geom::Point2D<FT>& ptin,
					 bool toworld ) const
{
    return Geom::Point2D<TT>( transformX<TT,FT>(ptin.x,toworld),
			      transformY<TT,FT>(ptin.y,toworld) );
}
