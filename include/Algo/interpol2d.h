#ifndef interpol2d_h
#define interpol2d_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Mar 2006
 RCS:		$Id: interpol2d.h,v 1.1 2006-03-14 14:58:51 cvsbert Exp $
________________________________________________________________________

*/

#include "undefval.h"


namespace Interpolate
{

/*!\brief Linear 2D interpolation. */

template <class T>
class LinearReg2D
{
public:

LinearReg2D() {}

LinearReg2D( const T* const* v )
{
    set( v[0][0], v[1][0], v[0][1], v[1][1] );
}

LinearReg2D( T v00, T v10, T v01, T v11 )
{
    set( v00, v10, v01, v11 );
}

inline void set( T v00, T v10, T v01, T v11 )
{
    a_[0] = v00;
    a_[1] = v10 - v00;
    a_[2] = v01 - v00;
    a_[3] = v11 + v00 - v10 - v01;
}

inline T apply( float x, float y ) const
{
    return a_[0] + a_[1] * x + a_[2] * y + a_[3] * x * y;
}

    T	a_[4];

};


template <class T>
inline T linearReg2D( T v00, T v10, T v01, T v11, float x, float y )
{
    return LinearReg2D<T>( v00, v10, v01, v11 ).apply ( x, y );
}


/*!\brief Linear 2D interpolation with standard undef handling.  */

template <class T>
class LinearReg2DWithUdf
{
public:

LinearReg2DWithUdf()	{}

LinearReg2DWithUdf( const T* const* v )
{
    set( v[0][0], v[1][0], v[0][1], v[1][1] );
}

LinearReg2DWithUdf( T v00, T v10, T v01, T v11 )
{
    set( v00, v10, v01, v11 );
}

inline void set( T v00, T v10, T v01, T v11 )
{
    u00_ = mIsUdf(v00);
    u10_ = mIsUdf(v10);
    u01_ = mIsUdf(v01);
    u11_ = mIsUdf(v11);
    haveudf_ = u00_ || u10_ || u01_ || u11_;

    if ( haveudf_ )
    {

#	define mFillIfUdf(nd,left,right,opp) \
	if ( u##nd##_ ) \
	{ \
	    if ( u##left##_ && u##right##_ ) \
		v##nd = v##opp; \
	    v##nd = u##left##_ || u##right##_ ? \
	    	   (u##right##_ ? v##left : v##right) \
				: (v##left + v##right) / 2; \
	}

	mFillIfUdf(00,01,10,11)
	mFillIfUdf(10,00,11,01)
	mFillIfUdf(01,11,00,10)
	mFillIfUdf(11,10,01,00)
    }

    intp_.set( v00, v10, v01, v11 );
}

inline T apply( float x, float y ) const
{
    // return undef if the nearest node is undef
    if ( haveudf_ && (
	 ( u00_ && x < 0.5 && y < 0.5 )
      || ( u10_ && x >= 0.5 && y < 0.5 )
      || ( u01_ && x < 0.5 && y >= 0.5 )
      || ( u11_ && x >= 0.5 && y >= 0.5 ) ) )
	return mUdf(T);

    return intp_.apply( x, y );
}

    LinearReg2D<T>	intp_;
    bool		haveudf_;
    bool		u00_;
    bool		u10_;
    bool		u01_;
    bool		u11_;

};


template <class T>
inline T linearReg2DWithUdf( T v00, T v10, T v01, T v11, float x, float y )
{
    return LinearReg2DWithUdf<T>( v00, v10, v01, v11 ).apply ( x, y );
}


/*!<\brief Interpolate 2D regularly sampled, using a 3rd order surface. */

template <class T>
class PolyReg2D
{
public:

PolyReg2D() {}

PolyReg2D( const T* const* v )
{
	set( v[0][1], v[0][2],
    v[1][0], v[1][1], v[1][2], v[1][3],
    v[2][0], v[2][1], v[2][2], v[2][3],
	     v[3][1], v[3][2] );
}

PolyReg2D( T vm10, T vm11,
   T v0m1, T v00,  T v01, T v02,
   T v1m1, T v10,  T v11, T v12,
	   T v20,  T v21 )
{
    set( vm10, vm11, v0m1, v00, v01, v02, v1m1, v10, v11, v12, v20, v21 );
}

inline void set( T vm10, T vm11,
	 T v0m1, T v00,  T v01, T v02,
	 T v1m1, T v10,  T v11, T v12,
		 T v20,  T v21 )
{
    linintp_.set( v00, v10, v01, v11 );
}

inline T apply( float x, float y ) const
{
    return linintp_.apply( x, y );
}

    // TODO: do it right
    LinearReg2D<T>	linintp_;

};


template <class T>
inline T polyReg2D( T vm10, T vm11, T v0m1, T v00, T v01, T v02,
	   T v1m1, T v10, T v11, T v12, T v20, T v21, float x, float y )
{
    return PolyReg2D<T>(vm10,vm11,v0m1,v00,v01,v02,v1m1,v10,v11,v12,v20,v21)
	  .apply( x, y );
}


/*!<\brief PolyReg2D which smoothly handles undefined values

  Note that this class _requires_ x and y to be between 0 and 1 for correct
  undef handling. Correct means: if the nearest sample is undefined, return
  undefined. Otherwise always return a value.

  */

template <class T>
class PolyReg2DWithUdf
{
public:

PolyReg2DWithUdf()	{}

PolyReg2DWithUdf( const T* const* v )
{
	set( v[0][1], v[0][2],
    v[1][0], v[1][1], v[1][2], v[1][3],
    v[2][0], v[2][1], v[2][2], v[2][3],
	     v[3][1], v[3][2] );
}

PolyReg2DWithUdf( T vm10, T vm11, T v0m1, T v00,  T v01, T v02,
		  T v1m1, T v10,  T v11, T v12, T v20,  T v21 )
{
    set( vm10, vm11, v0m1, v00, v01, v02, v1m1, v10, v11, v12, v20, v21 );
}

inline void set( T vm10, T vm11,
	 T v0m1, T v00,  T v01, T v02,
	 T v1m1, T v10,  T v11, T v12,
		 T v20,  T v21 )
{
    linintp_.set( v00, v10, v01, v11 );
}

inline T apply( float x, float y ) const
{
    return linintp_.apply( x, y );
}

    // TODO: do it right
    LinearReg2DWithUdf<T>	linintp_;

};


template <class T>
inline T polyReg2DWithUdf( T vm10, T vm11, T v0m1, T v00, T v01, T v02,
	   T v1m1, T v10, T v11, T v12, T v20, T v21, float x, float y )
{
    return PolyReg2DWithUdf<T>(vm10,vm11,v0m1,v00,v01,v02,v1m1,v10,v11,v12,v20,
	    			v21).apply( x, y );
}


} // namespace Interpolate

#endif
