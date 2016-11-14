#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

*/

#include "algomod.h"


/*!\brief Sets up a terrain polynomial for a 3x3 grid from 2D regularly
  sampled data.

After Zevenbergen and Thorne (1987), Earth Surface Processes and Landforms,
Vol 12, 47-56. Substitutes:

A=>ax2y2_ B=>ay2x_ C=>ax2y_ D=>ax2_ E=>ay2_ F=>axy_ G=>ax_ H=>ay_ I=>a0_

Z1 (-1, 1) => v[2]
Z2 ( 0, 1) => v[5]
Z3 ( 1, 1) => v[8]
Z4 (-1, 0) => v[1]
Z5 ( 0, 0) => v[4]
Z6 ( 1, 0) => v[7]
Z7 (-1,-1) => v[0]
Z8 ( 0,-1) => v[3]
Z9 ( 1,-1) => v[6]

Beware that direction, profileCurvature and planformCurvature can return undef.

*/

template <class T>
mClass(Algo) Terrain3x3
{
public:
		    Terrain3x3(T dist,const T* zvals=0);
    void	    set(const T*);
			//!< 0=[-1,-1] 1=[-1,0] 2=[-1,1]
			//!< 3=[ 0,-1] 4=[ 0,0] 5=[ 0,1]
			//!< 6=[ 1,-1] 7=[ 1,0] 8=[ 1,1]

    T		    valueAt(T x,T y) const;
    T		    slope() const;
    T		    direction() const;		//!< -pi < d < pi, or undef
    T		    profileCurvature() const;	//!< curv in slope direction
    T		    planformCurvature() const;	//!< curv perp to slope dir

    const T	d_, d2_;
    T		a0_, ax_, ay_, axy_, ax2_, ay2_, ax2y_, ay2x_, ax2y2_;

};


template <class T>
inline Terrain3x3<T>::Terrain3x3( T dist, const T* v )
    : d_(dist)
    , d2_(dist*dist)
{
    if ( v )
	set( v );
    else
	a0_ = ax_ = ay_ = axy_ = ax2_ = ay2_ = ax2y_ = ay2x_ = ax2y2_ = ((T)0);
}


template <class T>
inline void Terrain3x3<T>::set( const T* v )
{
    ax2y2_ = ( v[4] + (v[2]+v[8]+v[0]+v[6])/4
	    - (v[5]+v[1]+v[7]+v[3])/4 )				/ (d2_*d2_);
    ay2x_ = ( (v[2]+v[8]-v[0]-v[6])/4  + (v[3]-v[5])/2 )	/ (d2_*d_);
    ax2y_ = ( (v[8]+v[6]-v[2]-v[0])/4 + (v[1]-v[7])/2 )		/ (d2_*d_);
    ax2_  = ( (v[1]+v[7])/2 - v[4] )				/ (d2_);
    ay2_  = ( (v[5]+v[3])/2 - v[4] )				/ (d2_);
    axy_  = (  v[8]+v[0]-v[2]-v[6] )				/ (4*d2_);
    ax_   = (  v[7]-v[1] )					/ (2*d_);
    ay_   = (  v[5]-v[3] )					/ (2*d_);
    a0_   =    v[4];
}


template <class T>
inline T Terrain3x3<T>::valueAt( T x, T y ) const
{
    const T x2 = x * x;
    const T y2 = y * y;
    return a0_ + ax_*x + ay_*y + axy_*x*y
	      + ax2_*x2 + ay2_*y2
	      + ax2y_*x*y2 + ay2x_*y*x2
	      + ax2y2_*x2*y2;
}


template <class T>
inline T Terrain3x3<T>::slope() const
{
    return Math::Sqrt( ax_*ax_ + ay_*ay_ );
}


template <class T>
inline T Terrain3x3<T>::direction() const
{
    return Math::Atan2( -ay_, -ax_ );
}


template <class T>
inline T Terrain3x3<T>::profileCurvature() const
{
    const T ax2 = ax_ * ax_;
    const T ay2 = ay_ * ay_;
    const T divby = ax2 + ay2;
    return divby == 0 ? mUdf(T)
	 : 2 * (ax2_*ax2 + ay2_*ay2 + axy_*ax_*ay_) / divby;
}


template <class T>
inline T Terrain3x3<T>::planformCurvature() const
{
    const T ax2 = ax_ * ax_;
    const T ay2 = ay_ * ay_;
    const T divby = ax2 + ay2;
    return divby == 0 ? mUdf(T)
	 : 2 * (ax2_*ay2 + ay2_*ax2 - axy_*ax_*ay_) / divby;
}
