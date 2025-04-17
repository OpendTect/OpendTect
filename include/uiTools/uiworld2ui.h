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
			World2UiData(const uiSize&,const uiWorldRect&);
			World2UiData(const uiWorldRect&,const uiSize&);
    virtual		~World2UiData();

    bool		operator ==(const World2UiData&) const;
    bool		operator !=(const World2UiData&) const;

    const uiSize&	size() const		{ return sz_; }
    const uiWorldRect&	rect() const		{ return wr_; }

private:
    uiSize		sz_;
    uiWorldRect		wr_;
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
			uiWorld2Ui(const uiSize&,const uiWorldRect&);
			uiWorld2Ui(const uiRect&,const uiWorldRect&);
			uiWorld2Ui(const uiWorldRect&,const uiSize&);
			uiWorld2Ui(const World2UiData&);
    virtual		~uiWorld2Ui();

    bool		operator==(const uiWorld2Ui&) const;
    uiWorld2Ui&		operator=(const uiWorld2Ui&);

    void		set(const World2UiData&);
    void		set(const uiWorldRect&,const uiSize&);
    void		set(const uiSize& sz,const uiWorldRect&);
    void		set(const uiRect& rc,const uiWorldRect&);
    void		set(const uiRect& rc,const SurveyInfo&);
			//! Quite useful for survey level maps.

    void		setRemap(const uiSize&,const uiWorldRect&);
    void		setRemap(const uiRect&,const uiWorldRect&);
			//! For most cases we are dealing with cartesian coord.
			//! system. Just pass the X/Y range to set up
    void		setCartesianRemap(const uiSize&,
			    float minx,float maxx,float miny,float maxy);
    void		setCartesianRemap(const uiRect&,
			    float minx,float maxx,float miny,float maxy);

			//! The following reset...() functions must be called
			//! ONLY AFTER a call to set...() is made.
    void		resetUiRect(const uiRect&);

    const World2UiData&	world2UiData() const;


    uiWorldPoint	transform(const uiPoint&) const;
    uiWorldRect		transform(const uiRect&) const;
    uiPoint		transform(const uiWorldPoint&) const;
    uiRect		transform(const uiWorldRect&) const;
			// Since the compiler will be confused if two functions
			// only differ in return type, Geom::Point2D<float> is
			// set rather than be returned.
    void		transform(const uiPoint&,
				  Geom::Point2D<float>&) const;
    void		transform(const Geom::Point2D<float>&,
				  uiPoint&) const;

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

    World2UiData	w2ud_;

    uiWorldPoint	p0_;
    uiWorldPoint	fac_;

    uiWorldRect		wrdrect_; //!< the world rect used for coord. conv.
    uiSize		uisize_;
    uiPoint		uiorigin_;

private:
			void getAppropriateRange( float min, float max,
						 float& newmin, float& newmax );
};


template <class TT,class FT>
TT uiWorld2Ui::transformX( FT x, bool toworld ) const
{
    if ( toworld )
	return p0_.x_ + (x-uiorigin_.x_)*fac_.x_;
    else
	return (x-p0_.x_)/fac_.x_ + uiorigin_.x_;
}

template <class TT,class FT>
TT uiWorld2Ui::transformY( FT y, bool toworld ) const
{
    if ( toworld )
	return p0_.y_ + (y-uiorigin_.y_)*fac_.y_;
    else
	return (y-p0_.y_)/fac_.y_ + uiorigin_.y_;
}


template <class TT,class FT>
Geom::Point2D<TT> uiWorld2Ui::transform( const Geom::Point2D<FT>& ptin,
					 bool toworld ) const
{
    return Geom::Point2D<TT>( transformX<TT,FT>(ptin.x_,toworld),
                              transformY<TT,FT>(ptin.y_,toworld) );
}
