#ifndef array2dfunc_h
#define array2dfunc_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id$
________________________________________________________________________

*/

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

    inline RT		getValue(PT,PT) const;
    inline RT		getValue( const PT* p ) const
			{ return getValue(p[0],p[1]); }

protected:

    const Array2D<T>*	arr_;
    int			xsize_;
    int			ysize_;
    bool		hasudfs_;
};


template <class RT,class PT,class T> inline
RT Array2DFunc<RT,PT,T>::getValue( PT x, PT y ) const
{
    float xrelpos;
    const int prevxsample = getPrevSample( x, xsize_, xrelpos );
    if ( prevxsample<0 || prevxsample>=xsize_-1 ) return mUdf(RT);

    float yrelpos;
    const int prevysample = getPrevSample( y, ysize_, yrelpos );
    if ( prevysample<0  || prevysample>=ysize_-1) return mUdf(RT);

    if ( xsize_<4 || ysize_<4 || !prevxsample || !prevysample ||
	 prevxsample>=xsize_-2 || prevysample>=ysize_-2 )
    {
	if ( hasudfs_ )
	{
	    return Interpolate::linearReg2DWithUdf<T>(
				arr_->get( prevxsample, prevysample ),
				arr_->get( prevxsample, prevysample+1 ),
				arr_->get( prevxsample+1, prevysample ),
				arr_->get( prevxsample+1, prevysample+1),
				xrelpos, yrelpos );
	}

	return Interpolate::linearReg2D<T>(
			    arr_->get( prevxsample, prevysample ),
			    arr_->get( prevxsample, prevysample+1 ),
			    arr_->get( prevxsample+1, prevysample ),
			    arr_->get( prevxsample+1, prevysample+1 ),
			    xrelpos, yrelpos );
    }

    if ( hasudfs_ )
    {
	return Interpolate::polyReg2DWithUdf<T>(
		arr_->get( prevxsample-1, prevysample ),
		arr_->get( prevxsample-1, prevysample+1 ),
		arr_->get( prevxsample, prevysample-1 ),
		arr_->get( prevxsample, prevysample ),
		arr_->get( prevxsample, prevysample+1 ),
		arr_->get( prevxsample, prevysample+2 ),
		arr_->get( prevxsample+1, prevysample-1 ),
		arr_->get( prevxsample+1, prevysample ),
		arr_->get( prevxsample+1, prevysample+1 ),
		arr_->get( prevxsample+1, prevysample+2 ),
		arr_->get( prevxsample+2, prevysample ),
		arr_->get( prevxsample+2, prevysample+1 ),
		xrelpos, yrelpos );
    }
    

    return Interpolate::polyReg2D<T>( arr_->get( prevxsample-1, prevysample ),
	    arr_->get( prevxsample-1, prevysample+1 ),
	    arr_->get( prevxsample, prevysample-1 ),
	    arr_->get( prevxsample, prevysample ),
	    arr_->get( prevxsample, prevysample+1 ),
	    arr_->get( prevxsample, prevysample+2 ),
	    arr_->get( prevxsample+1, prevysample-1 ),
	    arr_->get( prevxsample+1, prevysample ),
	    arr_->get( prevxsample+1, prevysample+1 ),
	    arr_->get( prevxsample+1, prevysample+2 ),
	    arr_->get( prevxsample+2, prevysample ),
	    arr_->get( prevxsample+2, prevysample+1 ),
	    xrelpos, yrelpos );
}

#endif
