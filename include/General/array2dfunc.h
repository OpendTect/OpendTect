#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mathfunc.h"
#include "interpol2d.h"
#include "simpnumer.h"

/*!
\brief Adaptor to make an Array2D behave like a MathXYFunction. Will do linear
interpolation at the edges, and polynomial interpolation inside.
*/

template <class RT,class PT,class T>
mClass(General) Array2DFunc : public MathXYFunction<RT,PT>
{
public:
    inline void		set( const Array2D<T>& t, bool hasudfs )
			{
			    arr_ = &t;
			    xsize_ = arr_->info().getSize(0);
			    ysize_ = arr_->info().getSize(1);
			    hasudfs_ = hasudfs;
			}

    inline RT		getValue(PT,PT) const override;
    inline RT		getValue( const PT* p ) const
			{ return getValue(p[0],p[1]); }

protected:

    const Array2D<T>*	arr_ = nullptr;
    int			xsize_ = 0;
    int			ysize_ = 0;
    bool		hasudfs_ = true;
};


template <class RT,class PT,class T> inline
RT Array2DFunc<RT,PT,T>::getValue( PT x, PT y ) const
{
    float xrelpos;
    const int ix0 = Interpolate::getArrIdxPosition( x, xsize_, xrelpos );
    if ( ix0<0 || ix0>=xsize_-1 ) return mUdf(RT);

    float yrelpos;
    const int iy0 = Interpolate::getArrIdxPosition( y, ysize_, yrelpos );
    if ( iy0<0  || iy0>=ysize_-1) return mUdf(RT);

    if ( xsize_<4 || ysize_<4 || !ix0 || !iy0 ||
	 ix0>=xsize_-2 || iy0>=ysize_-2 )
    {
	if ( hasudfs_ )
	{
	    return Interpolate::linearReg2DWithUdf<T>(
				arr_->get( ix0, iy0 ),
				arr_->get( ix0, iy0+1 ),
				arr_->get( ix0+1, iy0 ),
				arr_->get( ix0+1, iy0+1),
				xrelpos, yrelpos );
	}

	return Interpolate::linearReg2D<T>(
			    arr_->get( ix0, iy0 ),
			    arr_->get( ix0, iy0+1 ),
			    arr_->get( ix0+1, iy0 ),
			    arr_->get( ix0+1, iy0+1 ),
			    xrelpos, yrelpos );
    }

    if ( hasudfs_ )
    {
	return Interpolate::polyReg2DWithUdf<T>(
		arr_->get( ix0-1, iy0 ),
		arr_->get( ix0-1, iy0+1 ),
		arr_->get( ix0, iy0-1 ),
		arr_->get( ix0, iy0 ),
		arr_->get( ix0, iy0+1 ),
		arr_->get( ix0, iy0+2 ),
		arr_->get( ix0+1, iy0-1 ),
		arr_->get( ix0+1, iy0 ),
		arr_->get( ix0+1, iy0+1 ),
		arr_->get( ix0+1, iy0+2 ),
		arr_->get( ix0+2, iy0 ),
		arr_->get( ix0+2, iy0+1 ),
		xrelpos, yrelpos );
    }


    return Interpolate::polyReg2D<T>( arr_->get( ix0-1, iy0 ),
	    arr_->get( ix0-1, iy0+1 ),
	    arr_->get( ix0, iy0-1 ),
	    arr_->get( ix0, iy0 ),
	    arr_->get( ix0, iy0+1 ),
	    arr_->get( ix0, iy0+2 ),
	    arr_->get( ix0+1, iy0-1 ),
	    arr_->get( ix0+1, iy0 ),
	    arr_->get( ix0+1, iy0+1 ),
	    arr_->get( ix0+1, iy0+2 ),
	    arr_->get( ix0+2, iy0 ),
	    arr_->get( ix0+2, iy0+1 ),
	    xrelpos, yrelpos );
}
